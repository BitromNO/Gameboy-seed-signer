#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "psbt_inspector.h"
#include "psbt_review.h"
#include "review_flags.h"
#include "wallet_policy.h"

static const char *version_name(PsbtVersion version) {
    switch (version) {
        case PSBT_VERSION_V0: return "v0";
        case PSBT_VERSION_V2: return "v2";
        default: return "unknown";
    }
}

static int hex_value(char character) {
    if (character >= '0' && character <= '9') return character - '0';
    if (character >= 'a' && character <= 'f') return character - 'a' + 10;
    if (character >= 'A' && character <= 'F') return character - 'A' + 10;
    return -1;
}

static int add_owned_script_hex(WalletPolicy *policy, const char *hex) {
    uint8_t script[WALLET_POLICY_MAX_SCRIPT_BYTES];
    size_t length = 0u;
    int high;
    int low;

    while (hex[length * 2u] != '\0') {
        if (hex[length * 2u + 1u] == '\0') return 0;
        high = hex_value(hex[length * 2u]);
        low = hex_value(hex[length * 2u + 1u]);
        if (high < 0 || low < 0 || length == sizeof(script)) return 0;
        script[length++] = (uint8_t)((high << 4) | low);
    }
    if (length == 0u) return 0;
    return wallet_policy_add_script(policy, script, length);
}

int main(int argc, char *argv[]) {
    FILE *file;
    long file_size;
    uint8_t *data;
    PsbtFileInfo info = { 0u, 0u, 0u, PSBT_VERSION_UNKNOWN, 0u, 0u, 0u, 0u };
    PsbtReview review;
    WalletPolicy policy = { 0u, { { 0u, { 0u } } } };
    PsbtStatus status;
    PsbtReviewStatus review_status;
    uint32_t review_flags;
    uint16_t output_index;
    int argument_index;
    const char *psbt_path;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s [--owned-script scriptpubkey-hex] unsigned.psbt\n", argv[0]);
        return 2;
    }
    argument_index = 1;
    while (argument_index + 1 < argc && strcmp(argv[argument_index], "--owned-script") == 0) {
        if (!add_owned_script_hex(&policy, argv[argument_index + 1])) {
            fprintf(stderr, "Invalid or oversized owned script\n");
            return 2;
        }
        argument_index += 2;
    }
    if (argument_index != argc - 1) {
        fprintf(stderr, "Usage: %s [--owned-script scriptpubkey-hex] unsigned.psbt\n", argv[0]);
        return 2;
    }
    psbt_path = argv[argument_index];
    file = fopen(psbt_path, "rb");
    if (file == NULL) {
        perror("Cannot open PSBT");
        return 2;
    }
    if (fseek(file, 0L, SEEK_END) != 0 || (file_size = ftell(file)) < 0 || fseek(file, 0L, SEEK_SET) != 0) {
        fclose(file);
        fprintf(stderr, "Cannot determine PSBT size\n");
        return 2;
    }
    if ((unsigned long)file_size > PSBT_MAX_BYTES) {
        fclose(file);
        fprintf(stderr, "PSBT exceeds %u-byte development limit\n", PSBT_MAX_BYTES);
        return 1;
    }
    data = malloc((size_t)file_size);
    if (data == NULL && file_size != 0L) {
        fclose(file);
        fprintf(stderr, "Cannot allocate PSBT buffer\n");
        return 2;
    }
    if ((size_t)file_size != fread(data, 1u, (size_t)file_size, file)) {
        free(data);
        fclose(file);
        fprintf(stderr, "Cannot read PSBT\n");
        return 2;
    }
    fclose(file);
    status = psbt_validate_envelope(data, (size_t)file_size, &info);
    free(data);
    if (status != PSBT_STATUS_OK) {
        fprintf(stderr, "Rejected: %s\n", psbt_status_message(status));
        return 1;
    }
    printf("PSBT version: %s\n", version_name(info.version));
    printf("File bytes: %u\n", info.byte_count);
    printf("Global records: %u\n", info.global_record_count);
    printf("Inputs: %u\n", info.input_count);
    printf("Outputs: %u\n", info.output_count);
    if (info.version == PSBT_VERSION_V0) printf("Output total (sats): %llu\n", (unsigned long long)info.total_output_sats);
    printf("Recognized outputs: %u\n", info.recognized_output_count);
    if (info.version == PSBT_VERSION_V0 || info.version == PSBT_VERSION_V2) {
        file = fopen(psbt_path, "rb");
        if (file == NULL) return 2;
        data = malloc((size_t)file_size);
        if (data == NULL && file_size != 0L) { fclose(file); return 2; }
        if ((size_t)file_size != fread(data, 1u, (size_t)file_size, file)) { free(data); fclose(file); return 2; }
        fclose(file);
        review_status = info.version == PSBT_VERSION_V0 ?
            psbt_parse_v0_review(data, (size_t)file_size, &review) :
            psbt_parse_v2_review(data, (size_t)file_size, &review);
        free(data);
        if (review_status != PSBT_REVIEW_OK) {
            fprintf(stderr, "Review unavailable: %s\n", psbt_review_status_message(review_status));
            return 1;
        }
        wallet_policy_mark_change(&policy, &review);
        for (output_index = 0u; output_index < review.output_count; output_index++) {
            printf("Output %u: %llu sats", (unsigned)(output_index + 1u), (unsigned long long)review.outputs[output_index].amount_sats);
            if (review.outputs[output_index].address[0] != '\0') printf(" -> %s", review.outputs[output_index].address);
            if (review.outputs[output_index].is_change) printf(" [CHANGE]");
            printf("\n");
        }
        if (info.version == PSBT_VERSION_V0) {
            printf("Transaction version: %u\n", review.transaction_version);
            printf("Locktime: %u\n", review.locktime);
        }
        if (review.fee_is_known) printf("Fee (sats): %llu\n", (unsigned long long)review.fee_sats);
        else printf("Fee: unavailable (input amounts incomplete)\n");
        review_flags = psbt_review_flags(&review, 0u);
        if (review_flags == 0u) printf("Warnings: none\n");
        else {
            printf("Warnings:");
            if (review_flags & REVIEW_FLAG_LOCKTIME) printf(" locktime");
            if (review_flags & REVIEW_FLAG_NONFINAL_SEQUENCE) printf(" non-final-sequence");
            if (review_flags & REVIEW_FLAG_UNKNOWN_OUTPUT) printf(" unknown-output");
            if (review_flags & REVIEW_FLAG_FEE_UNAVAILABLE) printf(" fee-unavailable");
            if (review_flags & REVIEW_FLAG_FEE_LIMIT) printf(" fee-limit");
            printf("\n");
        }
    }
    return 0;
}