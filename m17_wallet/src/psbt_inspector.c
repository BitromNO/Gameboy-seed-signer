#include "psbt_inspector.h"

static const uint8_t psbt_magic[] = { 0x70u, 0x73u, 0x62u, 0x74u, 0xFFu };

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

PsbtStatus psbt_validate_envelope(const uint8_t *data, size_t length, PsbtFileInfo *info) {
    size_t index;
    size_t offset;
    uint32_t key_length;
    uint32_t value_length;
    uint16_t record_count = 0u;
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
            }
            return PSBT_STATUS_OK;
        }
        if (key_length > length - offset) return PSBT_STATUS_TRUNCATED;
        offset += key_length;
        status = read_compact_size(data, length, &offset, &value_length);
        if (status != PSBT_STATUS_OK) return status;
        if (value_length > length - offset) return PSBT_STATUS_TRUNCATED;
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
    }
    return "Unknown PSBT status";
}