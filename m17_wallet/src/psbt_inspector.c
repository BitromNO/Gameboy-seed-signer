#include "psbt_inspector.h"

static const uint8_t psbt_magic[] = { 0x70u, 0x73u, 0x62u, 0x74u, 0xFFu };
#define PSBT_TRANSACTION_ITEM_LIMIT 1024u

static PsbtStatus read_compact_size(const uint8_t *data, size_t length, size_t *offset, uint32_t *value) {
    uint8_t prefix;
    uint32_t result;

    if (*offset >= length) return PSBT_STATUS_TRUNCATED;
    prefix = data[(*offset)++];
    if (prefix < 0xFDu) {
        *value = prefix;
        return PSBT_STATUS_OK;
    }
    if (prefix == 0xFDu) {
        if (length - *offset < 2u) return PSBT_STATUS_TRUNCATED;
        result = (uint32_t)data[*offset] | ((uint32_t)data[*offset + 1u] << 8);
        *offset += 2u;
        if (result < 0xFDu) return PSBT_STATUS_NON_CANONICAL_SIZE;
        *value = result;
        return PSBT_STATUS_OK;
    }
    if (prefix == 0xFEu) {
        if (length - *offset < 4u) return PSBT_STATUS_TRUNCATED;
        result = (uint32_t)data[*offset] | ((uint32_t)data[*offset + 1u] << 8) |
                 ((uint32_t)data[*offset + 2u] << 16) | ((uint32_t)data[*offset + 3u] << 24);
        *offset += 4u;
        if (result <= 0xFFFFu) return PSBT_STATUS_NON_CANONICAL_SIZE;
        *value = result;
        return PSBT_STATUS_OK;
    }
    return PSBT_STATUS_TOO_LARGE;
}

static PsbtStatus skip_bytes(size_t length, size_t *offset, uint32_t count) {
    if (count > length - *offset) return PSBT_STATUS_INVALID_UNSIGNED_TRANSACTION;
    *offset += count;
    return PSBT_STATUS_OK;
}

static uint64_t read_u64_le(const uint8_t *data) {
    return (uint64_t)data[0] | ((uint64_t)data[1] << 8) | ((uint64_t)data[2] << 16) | ((uint64_t)data[3] << 24) |
           ((uint64_t)data[4] << 32) | ((uint64_t)data[5] << 40) | ((uint64_t)data[6] << 48) | ((uint64_t)data[7] << 56);
}

static PsbtStatus parse_unsigned_transaction(const uint8_t *data, uint32_t length, PsbtFileInfo *info) {
    size_t offset = 0u;
    uint32_t input_count;
    uint32_t output_count;
    uint32_t script_length;
    uint32_t index;
    uint64_t total_output_sats = 0u;
    uint64_t amount;
    PsbtStatus status;

    if (length < 10u) return PSBT_STATUS_INVALID_UNSIGNED_TRANSACTION;
    offset = 4u;
    status = read_compact_size(data, length, &offset, &input_count);
    if (status != PSBT_STATUS_OK) return PSBT_STATUS_INVALID_UNSIGNED_TRANSACTION;
    if (input_count == 0u || input_count > PSBT_TRANSACTION_ITEM_LIMIT) return PSBT_STATUS_TRANSACTION_COUNT_LIMIT;
    for (index = 0u; index < input_count; index++) {
        status = skip_bytes(length, &offset, 36u);
        if (status != PSBT_STATUS_OK) return status;
        status = read_compact_size(data, length, &offset, &script_length);
        if (status != PSBT_STATUS_OK) return PSBT_STATUS_INVALID_UNSIGNED_TRANSACTION;
        status = skip_bytes(length, &offset, script_length);
        if (status != PSBT_STATUS_OK) return status;
        status = skip_bytes(length, &offset, 4u);
        if (status != PSBT_STATUS_OK) return status;
    }
    status = read_compact_size(data, length, &offset, &output_count);
    if (status != PSBT_STATUS_OK) return PSBT_STATUS_INVALID_UNSIGNED_TRANSACTION;
    if (output_count == 0u || output_count > PSBT_TRANSACTION_ITEM_LIMIT) return PSBT_STATUS_TRANSACTION_COUNT_LIMIT;
    for (index = 0u; index < output_count; index++) {
        if (length - offset < 8u) return PSBT_STATUS_INVALID_UNSIGNED_TRANSACTION;
        amount = read_u64_le(data + offset);
        if (UINT64_MAX - total_output_sats < amount) return PSBT_STATUS_INVALID_UNSIGNED_TRANSACTION;
        total_output_sats += amount;
        offset += 8u;
        status = read_compact_size(data, length, &offset, &script_length);
        if (status != PSBT_STATUS_OK) return PSBT_STATUS_INVALID_UNSIGNED_TRANSACTION;
        status = skip_bytes(length, &offset, script_length);
        if (status != PSBT_STATUS_OK) return status;
    }
    if (length - offset != 4u) return PSBT_STATUS_INVALID_UNSIGNED_TRANSACTION;
    if (info != 0) {
        info->input_count = (uint16_t)input_count;
        info->output_count = (uint16_t)output_count;
        info->total_output_sats = total_output_sats;
    }
    return PSBT_STATUS_OK;
}

PsbtStatus psbt_validate_envelope(const uint8_t *data, size_t length, PsbtFileInfo *info) {
    size_t index;
    size_t offset;
    uint32_t key_length;
    uint32_t value_length;
    uint16_t record_count = 0u;
    uint8_t key_type;
    uint8_t saw_unsigned_transaction = 0u;
    uint8_t saw_version = 0u;
    PsbtVersion version = PSBT_VERSION_UNKNOWN;
    PsbtStatus status;

    if (length == 0u) return PSBT_STATUS_EMPTY;
    if (length > PSBT_MAX_BYTES) return PSBT_STATUS_TOO_LARGE;
    if (data == 0 || length < sizeof(psbt_magic)) return PSBT_STATUS_BAD_MAGIC;
    for (index = 0u; index < sizeof(psbt_magic); index++) {
        if (data[index] != psbt_magic[index]) return PSBT_STATUS_BAD_MAGIC;
    }
    offset = sizeof(psbt_magic);
    while (offset < length) {
        status = read_compact_size(data, length, &offset, &key_length);
        if (status != PSBT_STATUS_OK) return status;
        if (key_length == 0u) {
            if (info != 0) {
                info->byte_count = (uint32_t)length;
                info->global_map_bytes = (uint32_t)(offset - sizeof(psbt_magic));
                info->global_record_count = record_count;
                info->version = version;
            }
            return PSBT_STATUS_OK;
        }
        if (key_length > length - offset) return PSBT_STATUS_TRUNCATED;
        key_type = data[offset];
        if (key_length == 1u && key_type == 0x00u) {
            if (saw_unsigned_transaction) return PSBT_STATUS_DUPLICATE_GLOBAL_FIELD;
            saw_unsigned_transaction = 1u;
            if (version == PSBT_VERSION_UNKNOWN) version = PSBT_VERSION_V0;
        }
        if (key_length == 1u && key_type == 0xFBu) {
            if (saw_version) return PSBT_STATUS_DUPLICATE_GLOBAL_FIELD;
            saw_version = 1u;
        }
        offset += key_length;
        status = read_compact_size(data, length, &offset, &value_length);
        if (status != PSBT_STATUS_OK) return status;
        if (value_length > length - offset) return PSBT_STATUS_TRUNCATED;
        if (key_length == 1u && key_type == 0x00u) {
            status = parse_unsigned_transaction(data + offset, value_length, info);
            if (status != PSBT_STATUS_OK) return status;
        }
        if (key_length == 1u && key_type == 0xFBu) {
            uint32_t declared_version;
            if (value_length != 4u) return PSBT_STATUS_INVALID_GLOBAL_VERSION;
            declared_version = (uint32_t)data[offset] | ((uint32_t)data[offset + 1u] << 8) |
                               ((uint32_t)data[offset + 2u] << 16) | ((uint32_t)data[offset + 3u] << 24);
            if (declared_version != 2u) return PSBT_STATUS_INVALID_GLOBAL_VERSION;
            if (version == PSBT_VERSION_V0) return PSBT_STATUS_INVALID_GLOBAL_VERSION;
            version = PSBT_VERSION_V2;
        }
        offset += value_length;
        if (record_count == UINT16_MAX) return PSBT_STATUS_TOO_LARGE;
        record_count++;
    }
    return PSBT_STATUS_MISSING_GLOBAL_END;
}

const char *psbt_status_message(PsbtStatus status) {
    switch (status) {
        case PSBT_STATUS_OK: return "PSBT envelope accepted";
        case PSBT_STATUS_EMPTY: return "PSBT file is empty";
        case PSBT_STATUS_TOO_LARGE: return "PSBT file exceeds development limit";
        case PSBT_STATUS_BAD_MAGIC: return "PSBT magic prefix is invalid";
        case PSBT_STATUS_TRUNCATED: return "PSBT map is truncated";
        case PSBT_STATUS_NON_CANONICAL_SIZE: return "PSBT CompactSize is non-canonical";
        case PSBT_STATUS_EMPTY_KEY: return "PSBT key is empty";
        case PSBT_STATUS_MISSING_GLOBAL_END: return "PSBT global map has no terminator";
        case PSBT_STATUS_DUPLICATE_GLOBAL_FIELD: return "PSBT repeats a global field";
        case PSBT_STATUS_INVALID_GLOBAL_VERSION: return "PSBT global version is unsupported or inconsistent";
        case PSBT_STATUS_INVALID_UNSIGNED_TRANSACTION: return "PSBT unsigned transaction is malformed";
        case PSBT_STATUS_TRANSACTION_COUNT_LIMIT: return "PSBT transaction count exceeds development limit";
    }
    return "Unknown PSBT status";
}