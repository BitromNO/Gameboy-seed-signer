#include "psbt_review.h"

#include <string.h>

static const uint8_t psbt_magic[] = { 0x70u, 0x73u, 0x62u, 0x74u, 0xFFu };

static PsbtReviewStatus read_compact_size(const uint8_t *data, size_t length, size_t *offset, uint32_t *value) {
    uint8_t prefix;
    uint32_t result;

    if (*offset >= length) return PSBT_REVIEW_MAP_TRUNCATED;
    prefix = data[(*offset)++];
    if (prefix < 0xFDu) { *value = prefix; return PSBT_REVIEW_OK; }
    if (prefix == 0xFDu) {
        if (length - *offset < 2u) return PSBT_REVIEW_MAP_TRUNCATED;
        result = (uint32_t)data[*offset] | ((uint32_t)data[*offset + 1u] << 8);
        *offset += 2u;
        if (result < 0xFDu) return PSBT_REVIEW_MAP_TRUNCATED;
        *value = result;
        return PSBT_REVIEW_OK;
    }
    if (prefix == 0xFEu) {
        if (length - *offset < 4u) return PSBT_REVIEW_MAP_TRUNCATED;
        result = (uint32_t)data[*offset] | ((uint32_t)data[*offset + 1u] << 8) |
                 ((uint32_t)data[*offset + 2u] << 16) | ((uint32_t)data[*offset + 3u] << 24);
        *offset += 4u;
        if (result <= 0xFFFFu) return PSBT_REVIEW_MAP_TRUNCATED;
        *value = result;
        return PSBT_REVIEW_OK;
    }
    return PSBT_REVIEW_MAP_TRUNCATED;
}

static uint64_t read_u64_le(const uint8_t *data) {
    return (uint64_t)data[0] | ((uint64_t)data[1] << 8) | ((uint64_t)data[2] << 16) | ((uint64_t)data[3] << 24) |
           ((uint64_t)data[4] << 32) | ((uint64_t)data[5] << 40) | ((uint64_t)data[6] << 48) | ((uint64_t)data[7] << 56);
}

static uint32_t read_u32_le(const uint8_t *data) {
    return (uint32_t)data[0] | ((uint32_t)data[1] << 8) | ((uint32_t)data[2] << 16) | ((uint32_t)data[3] << 24);
}

static PsbtReviewStatus skip_bytes(size_t length, size_t *offset, uint32_t count) {
    if (count > length - *offset) return PSBT_REVIEW_TRANSACTION_TRUNCATED;
    *offset += count;
    return PSBT_REVIEW_OK;
}

static PsbtReviewStatus parse_output(const uint8_t *data, size_t length, size_t *offset, PsbtOutputReview *output) {
    uint32_t script_length;
    PsbtReviewStatus status;

    if (length - *offset < 8u) return PSBT_REVIEW_TRANSACTION_TRUNCATED;
    output->amount_sats = read_u64_le(data + *offset);
    *offset += 8u;
    status = read_compact_size(data, length, offset, &script_length);
    if (status != PSBT_REVIEW_OK || script_length > length - *offset) return PSBT_REVIEW_TRANSACTION_TRUNCATED;
    output->script_length = script_length <= sizeof(output->script) ? (uint8_t)script_length : 0u;
    if (output->script_length != 0u) memcpy(output->script, data + *offset, output->script_length);
    output->is_change = 0u;
    output->type = bitcoin_classify_output_script(data + *offset, script_length);
    output->address[0] = '\0';
    if (output->type == BITCOIN_OUTPUT_P2PKH || output->type == BITCOIN_OUTPUT_P2SH) {
        (void)bitcoin_encode_mainnet_legacy_address(data + *offset, script_length, output->address, sizeof(output->address));
    } else if (output->type == BITCOIN_OUTPUT_P2WPKH || output->type == BITCOIN_OUTPUT_P2WSH || output->type == BITCOIN_OUTPUT_P2TR) {
        (void)bitcoin_encode_mainnet_segwit_address(data + *offset, script_length, output->address, sizeof(output->address));
    }
    return skip_bytes(length, offset, script_length);
}

static PsbtReviewStatus parse_unsigned_transaction(const uint8_t *data, size_t length, PsbtReview *review) {
    size_t offset = 0u;
    uint32_t input_count;
    uint32_t output_count;
    uint32_t script_length;
    uint32_t index;
    PsbtReviewStatus status;

    if (length < 10u) return PSBT_REVIEW_TRANSACTION_TRUNCATED;
    offset = 4u;
    status = read_compact_size(data, length, &offset, &input_count);
    if (status != PSBT_REVIEW_OK) return PSBT_REVIEW_TRANSACTION_TRUNCATED;
    if (input_count == 0u || input_count > PSBT_REVIEW_MAX_INPUTS) return PSBT_REVIEW_TRANSACTION_LIMIT;
    for (index = 0u; index < input_count; index++) {
        status = skip_bytes(length, &offset, 36u);
        if (status != PSBT_REVIEW_OK) return status;
        review->inputs[index].previous_output_index = read_u32_le(data + offset - 4u);
        status = read_compact_size(data, length, &offset, &script_length);
        if (status != PSBT_REVIEW_OK) return PSBT_REVIEW_TRANSACTION_TRUNCATED;
        status = skip_bytes(length, &offset, script_length);
        if (status != PSBT_REVIEW_OK) return status;
        status = skip_bytes(length, &offset, 4u);
        if (status != PSBT_REVIEW_OK) return status;
    }
    status = read_compact_size(data, length, &offset, &output_count);
    if (status != PSBT_REVIEW_OK) return PSBT_REVIEW_TRANSACTION_TRUNCATED;
    if (output_count == 0u || output_count > PSBT_REVIEW_MAX_OUTPUTS) return PSBT_REVIEW_TRANSACTION_LIMIT;
    for (index = 0u; index < output_count; index++) {
        status = parse_output(data, length, &offset, &review->outputs[index]);
        if (status != PSBT_REVIEW_OK) return status;
        if (UINT64_MAX - review->total_output_sats < review->outputs[index].amount_sats) return PSBT_REVIEW_TRANSACTION_TRUNCATED;
        review->total_output_sats += review->outputs[index].amount_sats;
    }
    if (length - offset != 4u) return PSBT_REVIEW_TRANSACTION_TRUNCATED;
    review->input_count = (uint16_t)input_count;
    review->output_count = (uint16_t)output_count;
    return PSBT_REVIEW_OK;
}

static PsbtReviewStatus parse_witness_utxo(const uint8_t *data, size_t length, uint64_t *amount) {
    size_t offset = 0u;
    uint32_t script_length;
    PsbtReviewStatus status;

    if (length < 9u) return PSBT_REVIEW_TRANSACTION_TRUNCATED;
    *amount = read_u64_le(data);
    offset = 8u;
    status = read_compact_size(data, length, &offset, &script_length);
    if (status != PSBT_REVIEW_OK || script_length > length - offset) return PSBT_REVIEW_TRANSACTION_TRUNCATED;
    offset += script_length;
    return offset == length ? PSBT_REVIEW_OK : PSBT_REVIEW_TRANSACTION_TRUNCATED;
}

static PsbtReviewStatus parse_non_witness_utxo(const uint8_t *data, size_t length, uint32_t wanted_output, uint64_t *amount) {
    size_t offset = 0u;
    uint32_t input_count;
    uint32_t output_count;
    uint32_t script_length;
    uint32_t index;
    PsbtReviewStatus status;

    if (length < 10u) return PSBT_REVIEW_TRANSACTION_TRUNCATED;
    offset = 4u;
    status = read_compact_size(data, length, &offset, &input_count);
    if (status != PSBT_REVIEW_OK) return PSBT_REVIEW_TRANSACTION_TRUNCATED;
    for (index = 0u; index < input_count; index++) {
        status = skip_bytes(length, &offset, 36u);
        if (status != PSBT_REVIEW_OK) return status;
        status = read_compact_size(data, length, &offset, &script_length);
        if (status != PSBT_REVIEW_OK) return PSBT_REVIEW_TRANSACTION_TRUNCATED;
        status = skip_bytes(length, &offset, script_length);
        if (status != PSBT_REVIEW_OK) return status;
        status = skip_bytes(length, &offset, 4u);
        if (status != PSBT_REVIEW_OK) return status;
    }
    status = read_compact_size(data, length, &offset, &output_count);
    if (status != PSBT_REVIEW_OK || wanted_output >= output_count) return PSBT_REVIEW_TRANSACTION_TRUNCATED;
    for (index = 0u; index < output_count; index++) {
        if (length - offset < 8u) return PSBT_REVIEW_TRANSACTION_TRUNCATED;
        if (index == wanted_output) *amount = read_u64_le(data + offset);
        offset += 8u;
        status = read_compact_size(data, length, &offset, &script_length);
        if (status != PSBT_REVIEW_OK) return PSBT_REVIEW_TRANSACTION_TRUNCATED;
        status = skip_bytes(length, &offset, script_length);
        if (status != PSBT_REVIEW_OK) return status;
    }
    return length - offset == 4u ? PSBT_REVIEW_OK : PSBT_REVIEW_TRANSACTION_TRUNCATED;
}

static PsbtReviewStatus skip_map(const uint8_t *data, size_t length, size_t *offset, uint8_t is_input, uint16_t input_index, PsbtReview *review) {
    uint32_t key_length;
    uint32_t value_length;
    uint8_t key_type;
    uint8_t saw_witness_utxo = 0u;
    uint8_t saw_non_witness_utxo = 0u;
    PsbtReviewStatus status;

    while (*offset < length) {
        status = read_compact_size(data, length, offset, &key_length);
        if (status != PSBT_REVIEW_OK) return status;
        if (key_length == 0u) return PSBT_REVIEW_OK;
        if (key_length > length - *offset) return PSBT_REVIEW_MAP_TRUNCATED;
        key_type = data[*offset];
        if (is_input && key_length == 1u && key_type == 0x01u) {
            if (saw_witness_utxo) return PSBT_REVIEW_DUPLICATE_FIELD;
            saw_witness_utxo = 1u;
        }
        if (is_input && key_length == 1u && key_type == 0x00u) {
            if (saw_non_witness_utxo) return PSBT_REVIEW_DUPLICATE_FIELD;
            saw_non_witness_utxo = 1u;
        }
        *offset += key_length;
        status = read_compact_size(data, length, offset, &value_length);
        if (status != PSBT_REVIEW_OK || value_length > length - *offset) return PSBT_REVIEW_MAP_TRUNCATED;
        if (is_input && key_length == 1u && key_type == 0x01u) {
            uint64_t amount;
            status = parse_witness_utxo(data + *offset, value_length, &amount);
            if (status != PSBT_REVIEW_OK) return status;
            if (UINT64_MAX - review->total_input_sats < amount) return PSBT_REVIEW_TRANSACTION_TRUNCATED;
            if (review->inputs[input_index].amount_is_known) return PSBT_REVIEW_DUPLICATE_FIELD;
            review->inputs[input_index].amount_sats = amount;
            review->inputs[input_index].amount_is_known = 1u;
        }
        if (is_input && key_length == 1u && key_type == 0x00u) {
            uint64_t amount;
            status = parse_non_witness_utxo(data + *offset, value_length, review->inputs[input_index].previous_output_index, &amount);
            if (status != PSBT_REVIEW_OK) return status;
            if (review->inputs[input_index].amount_is_known) return PSBT_REVIEW_DUPLICATE_FIELD;
            review->inputs[input_index].amount_sats = amount;
            review->inputs[input_index].amount_is_known = 1u;
        }
        *offset += value_length;
    }
    return PSBT_REVIEW_MAP_TRUNCATED;
}

PsbtReviewStatus psbt_parse_v0_review(const uint8_t *data, size_t length, PsbtReview *review) {
    size_t offset = sizeof(psbt_magic);
    uint32_t key_length;
    uint32_t value_length;
    uint8_t key_type;
    const uint8_t *unsigned_transaction = 0;
    uint32_t unsigned_transaction_length = 0u;
    uint8_t saw_unsigned_transaction = 0u;
    uint16_t index;
    PsbtReviewStatus status;

    if (data == 0 || review == 0 || length < sizeof(psbt_magic) || memcmp(data, psbt_magic, sizeof(psbt_magic)) != 0) return PSBT_REVIEW_ENVELOPE_REJECTED;
    memset(review, 0, sizeof(*review));
    review->version = PSBT_VERSION_V0;
    while (offset < length) {
        status = read_compact_size(data, length, &offset, &key_length);
        if (status != PSBT_REVIEW_OK) return status;
        if (key_length == 0u) break;
        if (key_length > length - offset) return PSBT_REVIEW_MAP_TRUNCATED;
        key_type = data[offset];
        if (key_length == 1u && key_type == 0x00u) {
            if (saw_unsigned_transaction) return PSBT_REVIEW_DUPLICATE_FIELD;
            saw_unsigned_transaction = 1u;
        }
        offset += key_length;
        status = read_compact_size(data, length, &offset, &value_length);
        if (status != PSBT_REVIEW_OK || value_length > length - offset) return PSBT_REVIEW_MAP_TRUNCATED;
        if (key_length == 1u && key_type == 0x00u) {
            unsigned_transaction = data + offset;
            unsigned_transaction_length = value_length;
        }
        offset += value_length;
    }
    if (!saw_unsigned_transaction || unsigned_transaction == 0) return PSBT_REVIEW_UNSUPPORTED_VERSION;
    status = parse_unsigned_transaction(unsigned_transaction, unsigned_transaction_length, review);
    if (status != PSBT_REVIEW_OK) return status;
    for (index = 0u; index < review->input_count; index++) {
        status = skip_map(data, length, &offset, 1u, index, review);
        if (status != PSBT_REVIEW_OK) return status;
    }
    for (index = 0u; index < review->output_count; index++) {
        status = skip_map(data, length, &offset, 0u, 0u, review);
        if (status != PSBT_REVIEW_OK) return status;
    }
    if (offset != length) return PSBT_REVIEW_MAP_TRUNCATED;
    for (index = 0u; index < review->input_count; index++) {
        if (review->inputs[index].amount_is_known) {
            if (UINT64_MAX - review->total_input_sats < review->inputs[index].amount_sats) return PSBT_REVIEW_TRANSACTION_TRUNCATED;
            review->total_input_sats += review->inputs[index].amount_sats;
            review->known_input_amount_count++;
        }
    }
    if (review->known_input_amount_count == review->input_count && review->total_input_sats >= review->total_output_sats) {
        review->fee_sats = review->total_input_sats - review->total_output_sats;
        review->fee_is_known = 1u;
    }
    return PSBT_REVIEW_OK;
}

const char *psbt_review_status_message(PsbtReviewStatus status) {
    switch (status) {
        case PSBT_REVIEW_OK: return "PSBT review parsed";
        case PSBT_REVIEW_ENVELOPE_REJECTED: return "PSBT envelope rejected";
        case PSBT_REVIEW_UNSUPPORTED_VERSION: return "PSBT is not reviewable as v0";
        case PSBT_REVIEW_TRANSACTION_TRUNCATED: return "Transaction data is malformed";
        case PSBT_REVIEW_TRANSACTION_LIMIT: return "Transaction exceeds review limit";
        case PSBT_REVIEW_MAP_TRUNCATED: return "PSBT map is malformed";
        case PSBT_REVIEW_DUPLICATE_FIELD: return "PSBT repeats a critical field";
        case PSBT_REVIEW_UNSUPPORTED_INPUT_AMOUNT: return "PSBT input amount is unavailable";
    }
    return "Unknown PSBT review status";
}