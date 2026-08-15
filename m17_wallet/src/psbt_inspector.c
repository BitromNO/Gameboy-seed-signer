#include "psbt_inspector.h"
#include "sha256.h"

static const char bech32_charset[] = "qpzry9x8gf2tvdw0s3jn54khce6mua7l";
static const char base58_charset[] = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

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

BitcoinOutputType bitcoin_classify_output_script(const uint8_t *script, size_t length) {
    if (script == 0) return BITCOIN_OUTPUT_UNKNOWN;
    if (length >= 1u && script[0] == 0x6Au) return BITCOIN_OUTPUT_OP_RETURN;
    if (length == 25u && script[0] == 0x76u && script[1] == 0xA9u && script[2] == 0x14u && script[23] == 0x88u && script[24] == 0xACu) return BITCOIN_OUTPUT_P2PKH;
    if (length == 23u && script[0] == 0xA9u && script[1] == 0x14u && script[22] == 0x87u) return BITCOIN_OUTPUT_P2SH;
    if (length == 22u && script[0] == 0x00u && script[1] == 0x14u) return BITCOIN_OUTPUT_P2WPKH;
    if (length == 34u && script[0] == 0x00u && script[1] == 0x20u) return BITCOIN_OUTPUT_P2WSH;
    if (length == 34u && script[0] == 0x51u && script[1] == 0x20u) return BITCOIN_OUTPUT_P2TR;
    return BITCOIN_OUTPUT_UNKNOWN;
}

static uint32_t bech32_polymod_step(uint32_t checksum, uint8_t value) {
    static const uint32_t generator[5] = { 0x3B6A57B2u, 0x26508E6Du, 0x1EA119FAu, 0x3D4233DDu, 0x2A1462B3u };
    uint8_t index;
    uint32_t top = checksum >> 25;

    checksum = ((checksum & 0x1FFFFFFu) << 5) ^ value;
    for (index = 0u; index < 5u; index++) {
        if ((top >> index) & 1u) checksum ^= generator[index];
    }
    return checksum;
}

static uint32_t bech32_hrp_polymod(const char *hrp) {
    uint32_t checksum = 1u;
    size_t index;

    for (index = 0u; hrp[index] != '\0'; index++) checksum = bech32_polymod_step(checksum, (uint8_t)(hrp[index] >> 5));
    checksum = bech32_polymod_step(checksum, 0u);
    for (index = 0u; hrp[index] != '\0'; index++) checksum = bech32_polymod_step(checksum, (uint8_t)(hrp[index] & 31));
    return checksum;
}

int bitcoin_encode_mainnet_segwit_address(const uint8_t *script, size_t length, char *output, size_t output_capacity) {
    uint8_t data[66];
    uint8_t version;
    uint8_t program_length;
    uint32_t accumulator = 0u;
    uint8_t bits = 0u;
    uint8_t data_length = 1u;
    uint8_t byte_index;
    uint8_t checksum_index;
    uint32_t polymod;
    uint32_t encoding_constant;
    size_t output_index = 0u;

    if (script == 0 || output == 0 || length < 4u) return 0;
    if (script[0] == 0x00u) version = 0u;
    else if (script[0] >= 0x51u && script[0] <= 0x60u) version = (uint8_t)(script[0] - 0x50u);
    else return 0;
    program_length = script[1];
    if ((size_t)program_length + 2u != length || program_length < 2u || program_length > 40u) return 0;
    if (version == 0u && program_length != 20u && program_length != 32u) return 0;
    data[0] = version;
    for (byte_index = 0u; byte_index < program_length; byte_index++) {
        accumulator = (accumulator << 8) | script[byte_index + 2u];
        bits = (uint8_t)(bits + 8u);
        while (bits >= 5u) {
            bits = (uint8_t)(bits - 5u);
            data[data_length++] = (uint8_t)((accumulator >> bits) & 31u);
        }
    }
    if (bits != 0u) data[data_length++] = (uint8_t)((accumulator << (5u - bits)) & 31u);
    if (output_capacity < (size_t)data_length + 9u || (size_t)data_length + 9u > 90u) return 0;
    output[output_index++] = 'b';
    output[output_index++] = 'c';
    output[output_index++] = '1';
    for (byte_index = 0u; byte_index < data_length; byte_index++) output[output_index++] = bech32_charset[data[byte_index]];
    polymod = bech32_hrp_polymod("bc");
    for (byte_index = 0u; byte_index < data_length; byte_index++) polymod = bech32_polymod_step(polymod, data[byte_index]);
    for (checksum_index = 0u; checksum_index < 6u; checksum_index++) polymod = bech32_polymod_step(polymod, 0u);
    encoding_constant = version == 0u ? 1u : 0x2BC830A3u;
    polymod ^= encoding_constant;
    for (checksum_index = 0u; checksum_index < 6u; checksum_index++) output[output_index++] = bech32_charset[(polymod >> (5u * (5u - checksum_index))) & 31u];
    output[output_index] = '\0';
    return 1;
}

int bitcoin_encode_mainnet_legacy_address(const uint8_t *script, size_t length, char *output, size_t output_capacity) {
    uint8_t payload[25];
    uint8_t first_hash[32];
    uint8_t second_hash[32];
    uint8_t encoded[35];
    uint8_t working[25];
    uint8_t length_encoded = 0u;
    uint8_t leading_zeroes = 0u;
    uint8_t original_zeroes;
    uint8_t index;
    uint8_t quotient;
    uint16_t remainder;
    size_t output_index = 0u;

    if (script == 0 || output == 0 || output_capacity < 35u) return 0;
    if (bitcoin_classify_output_script(script, length) == BITCOIN_OUTPUT_P2PKH) payload[0] = 0x00u;
    else if (bitcoin_classify_output_script(script, length) == BITCOIN_OUTPUT_P2SH) payload[0] = 0x05u;
    else return 0;
    for (index = 0u; index < 20u; index++) payload[index + 1u] = script[index + 3u];
    sha256_digest(payload, 21u, first_hash);
    sha256_digest(first_hash, sizeof(first_hash), second_hash);
    for (index = 0u; index < 4u; index++) payload[index + 21u] = second_hash[index];
    for (index = 0u; index < sizeof(payload); index++) working[index] = payload[index];
    while (leading_zeroes < sizeof(working) && working[leading_zeroes] == 0u) leading_zeroes++;
    original_zeroes = leading_zeroes;
    while (leading_zeroes < sizeof(working)) {
        remainder = 0u;
        for (index = leading_zeroes; index < sizeof(working); index++) {
            remainder = (uint16_t)(remainder * 256u + working[index]);
            quotient = (uint8_t)(remainder / 58u);
            remainder %= 58u;
            working[index] = quotient;
        }
        encoded[length_encoded++] = (uint8_t)base58_charset[remainder];
        while (leading_zeroes < sizeof(working) && working[leading_zeroes] == 0u) leading_zeroes++;
    }
    while (original_zeroes-- != 0u) encoded[length_encoded++] = '1';
    if ((size_t)length_encoded + 1u > output_capacity) return 0;
    while (length_encoded != 0u) output[output_index++] = (char)encoded[--length_encoded];
    output[output_index] = '\0';
    return 1;
}

static PsbtStatus parse_unsigned_transaction(const uint8_t *data, uint32_t length, PsbtFileInfo *info) {
    size_t offset = 0u;
    uint32_t input_count;
    uint32_t output_count;
    uint32_t script_length;
    uint32_t index;
    uint16_t recognized_output_count = 0u;
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
        if (script_length > length - offset) return PSBT_STATUS_INVALID_UNSIGNED_TRANSACTION;
        if (bitcoin_classify_output_script(data + offset, script_length) != BITCOIN_OUTPUT_UNKNOWN) recognized_output_count++;
        status = skip_bytes(length, &offset, script_length);
        if (status != PSBT_STATUS_OK) return status;
    }
    if (length - offset != 4u) return PSBT_STATUS_INVALID_UNSIGNED_TRANSACTION;
    if (info != 0) {
        info->input_count = (uint16_t)input_count;
        info->output_count = (uint16_t)output_count;
        info->recognized_output_count = recognized_output_count;
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
    uint8_t saw_input_count = 0u;
    uint8_t saw_output_count = 0u;
    uint32_t v2_input_count = 0u;
    uint32_t v2_output_count = 0u;
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
            if (version == PSBT_VERSION_V2 && (!saw_input_count || !saw_output_count)) return PSBT_STATUS_MISSING_V2_COUNTS;
            if (info != 0) {
                info->byte_count = (uint32_t)length;
                info->global_map_bytes = (uint32_t)(offset - sizeof(psbt_magic));
                info->global_record_count = record_count;
                info->version = version;
                if (version == PSBT_VERSION_V2) {
                    info->input_count = (uint16_t)v2_input_count;
                    info->output_count = (uint16_t)v2_output_count;
                }
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
        if (key_length == 1u && key_type == 0x04u) {
            if (saw_input_count) return PSBT_STATUS_DUPLICATE_GLOBAL_FIELD;
            saw_input_count = 1u;
        }
        if (key_length == 1u && key_type == 0x05u) {
            if (saw_output_count) return PSBT_STATUS_DUPLICATE_GLOBAL_FIELD;
            saw_output_count = 1u;
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
        if (key_length == 1u && (key_type == 0x04u || key_type == 0x05u)) {
            size_t count_offset = offset;
            uint32_t decoded_count;
            status = read_compact_size(data, offset + value_length, &count_offset, &decoded_count);
            if (status != PSBT_STATUS_OK || count_offset != offset + value_length) return PSBT_STATUS_INVALID_UNSIGNED_TRANSACTION;
            if (decoded_count == 0u || decoded_count > PSBT_TRANSACTION_ITEM_LIMIT) return PSBT_STATUS_TRANSACTION_COUNT_LIMIT;
            if (key_type == 0x04u) v2_input_count = decoded_count;
            else v2_output_count = decoded_count;
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
        case PSBT_STATUS_MISSING_V2_COUNTS: return "PSBT v2 lacks transaction counts";
    }
    return "Unknown PSBT status";
}