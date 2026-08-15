#include <assert.h>
#include <stdint.h>

#include "psbt_inspector.h"

int main(void) {
    static const uint8_t valid_psbt_v0[] = {
        0x70u, 0x73u, 0x62u, 0x74u, 0xFFu, 0x01u, 0x00u, 0x3Cu,
        0x01u, 0x00u, 0x00u, 0x00u, 0x01u,
        0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
        0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
        0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
        0x00u, 0x00u, 0x00u, 0x00u,
        0x00u, 0x00u, 0x00u, 0x00u, 0xFFu, 0xFFu, 0xFFu, 0xFFu,
        0x00u, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0x01u,
        0xE8u, 0x03u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
        0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u
    };
    static const uint8_t valid_psbt_v2[] = {
        0x70u, 0x73u, 0x62u, 0x74u, 0xFFu,
        0x01u, 0xFBu, 0x04u, 0x02u, 0x00u, 0x00u, 0x00u,
        0x01u, 0x04u, 0x01u, 0x01u,
        0x01u, 0x05u, 0x01u, 0x01u,
        0x00u
    };
    static const uint8_t duplicate_version[] = { 0x70u, 0x73u, 0x62u, 0x74u, 0xFFu, 0x01u, 0xFBu, 0x04u, 0x02u, 0x00u, 0x00u, 0x00u, 0x01u, 0xFBu, 0x04u, 0x02u, 0x00u, 0x00u, 0x00u, 0x00u };
    static const uint8_t invalid_version[] = { 0x70u, 0x73u, 0x62u, 0x74u, 0xFFu, 0x01u, 0xFBu, 0x04u, 0x01u, 0x00u, 0x00u, 0x00u, 0x00u };
    static const uint8_t missing_v2_counts[] = { 0x70u, 0x73u, 0x62u, 0x74u, 0xFFu, 0x01u, 0xFBu, 0x04u, 0x02u, 0x00u, 0x00u, 0x00u, 0x00u };
    static const uint8_t invalid_psbt[] = { 0x70u, 0x73u, 0x62u, 0x74u, 0x00u };
    static const uint8_t truncated_psbt[] = { 0x70u, 0x73u, 0x62u, 0x74u, 0xFFu, 0x02u, 0x00u };
    static const uint8_t non_canonical_psbt[] = { 0x70u, 0x73u, 0x62u, 0x74u, 0xFFu, 0xFDu, 0x01u, 0x00u };
    static const uint8_t unterminated_psbt[] = { 0x70u, 0x73u, 0x62u, 0x74u, 0xFFu, 0x01u, 0xFCu, 0x00u };
    static const uint8_t p2pkh[] = { 0x76u, 0xA9u, 0x14u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0x88u, 0xACu };
    static const uint8_t p2wpkh[] = { 0x00u, 0x14u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u };
    static const uint8_t p2tr[] = { 0x51u, 0x20u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u };
    static const uint8_t op_return[] = { 0x6Au, 0x02u, 0x01u, 0x02u };
    PsbtFileInfo info = { 0u, 0u, 0u, PSBT_VERSION_UNKNOWN, 0u, 0u, 0u, 0u };

    assert(psbt_validate_envelope(valid_psbt_v0, sizeof(valid_psbt_v0), &info) == PSBT_STATUS_OK);
    assert(info.byte_count == sizeof(valid_psbt_v0));
    assert(info.global_map_bytes == 64u);
    assert(info.global_record_count == 1u);
    assert(info.version == PSBT_VERSION_V0);
    assert(info.input_count == 1u);
    assert(info.output_count == 1u);
    assert(info.recognized_output_count == 0u);
    assert(info.total_output_sats == 1000u);
    assert(bitcoin_classify_output_script(p2pkh, sizeof(p2pkh)) == BITCOIN_OUTPUT_P2PKH);
    assert(bitcoin_classify_output_script(p2wpkh, sizeof(p2wpkh)) == BITCOIN_OUTPUT_P2WPKH);
    assert(bitcoin_classify_output_script(p2tr, sizeof(p2tr)) == BITCOIN_OUTPUT_P2TR);
    assert(bitcoin_classify_output_script(op_return, sizeof(op_return)) == BITCOIN_OUTPUT_OP_RETURN);
    assert(psbt_validate_envelope(valid_psbt_v2, sizeof(valid_psbt_v2), &info) == PSBT_STATUS_OK);
    assert(info.version == PSBT_VERSION_V2);
    assert(info.input_count == 1u);
    assert(info.output_count == 1u);
    assert(psbt_validate_envelope(duplicate_version, sizeof(duplicate_version), &info) == PSBT_STATUS_DUPLICATE_GLOBAL_FIELD);
    assert(psbt_validate_envelope(invalid_version, sizeof(invalid_version), &info) == PSBT_STATUS_INVALID_GLOBAL_VERSION);
    assert(psbt_validate_envelope(missing_v2_counts, sizeof(missing_v2_counts), &info) == PSBT_STATUS_MISSING_V2_COUNTS);
    assert(psbt_validate_envelope(invalid_psbt, sizeof(invalid_psbt), &info) == PSBT_STATUS_BAD_MAGIC);
    assert(psbt_validate_envelope(truncated_psbt, sizeof(truncated_psbt), &info) == PSBT_STATUS_TRUNCATED);
    assert(psbt_validate_envelope(non_canonical_psbt, sizeof(non_canonical_psbt), &info) == PSBT_STATUS_NON_CANONICAL_SIZE);
    assert(psbt_validate_envelope(unterminated_psbt, sizeof(unterminated_psbt), &info) == PSBT_STATUS_MISSING_GLOBAL_END);
    assert(psbt_validate_envelope(0, 0u, &info) == PSBT_STATUS_EMPTY);
    assert(psbt_validate_envelope(valid_psbt_v0, PSBT_MAX_BYTES + 1u, &info) == PSBT_STATUS_TOO_LARGE);
    return 0;
}