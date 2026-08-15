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
    static const uint8_t valid_psbt_v2[] = { 0x70u, 0x73u, 0x62u, 0x74u, 0xFFu, 0x01u, 0xFBu, 0x04u, 0x02u, 0x00u, 0x00u, 0x00u, 0x00u };
    static const uint8_t duplicate_version[] = { 0x70u, 0x73u, 0x62u, 0x74u, 0xFFu, 0x01u, 0xFBu, 0x04u, 0x02u, 0x00u, 0x00u, 0x00u, 0x01u, 0xFBu, 0x04u, 0x02u, 0x00u, 0x00u, 0x00u, 0x00u };
    static const uint8_t invalid_version[] = { 0x70u, 0x73u, 0x62u, 0x74u, 0xFFu, 0x01u, 0xFBu, 0x04u, 0x01u, 0x00u, 0x00u, 0x00u, 0x00u };
    static const uint8_t invalid_psbt[] = { 0x70u, 0x73u, 0x62u, 0x74u, 0x00u };
    static const uint8_t truncated_psbt[] = { 0x70u, 0x73u, 0x62u, 0x74u, 0xFFu, 0x02u, 0x00u };
    static const uint8_t non_canonical_psbt[] = { 0x70u, 0x73u, 0x62u, 0x74u, 0xFFu, 0xFDu, 0x01u, 0x00u };
    static const uint8_t unterminated_psbt[] = { 0x70u, 0x73u, 0x62u, 0x74u, 0xFFu, 0x01u, 0xFCu, 0x00u };
    PsbtFileInfo info = { 0u, 0u, 0u, PSBT_VERSION_UNKNOWN, 0u, 0u, 0u };

    assert(psbt_validate_envelope(valid_psbt_v0, sizeof(valid_psbt_v0), &info) == PSBT_STATUS_OK);
    assert(info.byte_count == sizeof(valid_psbt_v0));
    assert(info.global_map_bytes == 64u);
    assert(info.global_record_count == 1u);
    assert(info.version == PSBT_VERSION_V0);
    assert(info.input_count == 1u);
    assert(info.output_count == 1u);
    assert(info.total_output_sats == 1000u);
    assert(psbt_validate_envelope(valid_psbt_v2, sizeof(valid_psbt_v2), &info) == PSBT_STATUS_OK);
    assert(info.version == PSBT_VERSION_V2);
    assert(psbt_validate_envelope(duplicate_version, sizeof(duplicate_version), &info) == PSBT_STATUS_DUPLICATE_GLOBAL_FIELD);
    assert(psbt_validate_envelope(invalid_version, sizeof(invalid_version), &info) == PSBT_STATUS_INVALID_GLOBAL_VERSION);
    assert(psbt_validate_envelope(invalid_psbt, sizeof(invalid_psbt), &info) == PSBT_STATUS_BAD_MAGIC);
    assert(psbt_validate_envelope(truncated_psbt, sizeof(truncated_psbt), &info) == PSBT_STATUS_TRUNCATED);
    assert(psbt_validate_envelope(non_canonical_psbt, sizeof(non_canonical_psbt), &info) == PSBT_STATUS_NON_CANONICAL_SIZE);
    assert(psbt_validate_envelope(unterminated_psbt, sizeof(unterminated_psbt), &info) == PSBT_STATUS_MISSING_GLOBAL_END);
    assert(psbt_validate_envelope(0, 0u, &info) == PSBT_STATUS_EMPTY);
    assert(psbt_validate_envelope(valid_psbt_v0, PSBT_MAX_BYTES + 1u, &info) == PSBT_STATUS_TOO_LARGE);
    return 0;
}