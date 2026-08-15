#include <stdio.h>
#include <stdlib.h>

#include "psbt_inspector.h"
#include "psbt_review.h"

static const char *version_name(PsbtVersion version) {
    switch (version) {
        case PSBT_VERSION_V0: return "v0";
        case PSBT_VERSION_V2: return "v2";
        default: return "unknown";
    }
}

int main(int argc, char *argv[]) {
    FILE *file;
    long file_size;
    uint8_t *data;
    PsbtFileInfo info = { 0u, 0u, 0u, PSBT_VERSION_UNKNOWN, 0u, 0u, 0u, 0u };
    PsbtReview review;
    PsbtStatus status;
    PsbtReviewStatus review_status;
    uint16_t output_index;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s unsigned.psbt\n", argv[0]);
        return 2;
    }
    file = fopen(argv[1], "rb");
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
    if (info.version == PSBT_VERSION_V0) {
        file = fopen(argv[1], "rb");
        if (file == NULL) return 2;
        data = malloc((size_t)file_size);
        if (data == NULL && file_size != 0L) { fclose(file); return 2; }
        if ((size_t)file_size != fread(data, 1u, (size_t)file_size, file)) { free(data); fclose(file); return 2; }
        fclose(file);
        review_status = psbt_parse_v0_review(data, (size_t)file_size, &review);
        free(data);
        if (review_status != PSBT_REVIEW_OK) {
            fprintf(stderr, "Review unavailable: %s\n", psbt_review_status_message(review_status));
            return 1;
        }
        for (output_index = 0u; output_index < review.output_count; output_index++) {
            printf("Output %u: %llu sats", (unsigned)(output_index + 1u), (unsigned long long)review.outputs[output_index].amount_sats);
            if (review.outputs[output_index].address[0] != '\0') printf(" -> %s", review.outputs[output_index].address);
            printf("\n");
        }
        if (review.fee_is_known) printf("Fee (sats): %llu\n", (unsigned long long)review.fee_sats);
        else printf("Fee: unavailable (input amounts incomplete)\n");
    }
    return 0;
}