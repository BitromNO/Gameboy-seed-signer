#include <assert.h>
#include <stdint.h>

#include "psbt_inspector.h"

int main(void) {
    static const uint8_t valid_psbt[] = { 0x70u, 0x73u, 0x62u, 0x74u, 0xFFu, 0x01u, 0x00u, 0x01u, 0x01u, 0x00u };
    static const uint8_t invalid_psbt[] = { 0x70u, 0x73u, 0x62u, 0x74u, 0x00u };
    static const uint8_t truncated_psbt[] = { 0x70u, 0x73u, 0x62u, 0x74u, 0xFFu, 0x02u, 0x00u };
    static const uint8_t non_canonical_psbt[] = { 0x70u, 0x73u, 0x62u, 0x74u, 0xFFu, 0xFDu, 0x01u, 0x00u };
    static const uint8_t unterminated_psbt[] = { 0x70u, 0x73u, 0x62u, 0x74u, 0xFFu, 0x01u, 0x00u, 0x00u };
    PsbtFileInfo info = { 0u, 0u, 0u };

    assert(psbt_validate_envelope(valid_psbt, sizeof(valid_psbt), &info) == PSBT_STATUS_OK);
    assert(info.byte_count == sizeof(valid_psbt));
    assert(info.global_map_bytes == 5u);
    assert(info.global_record_count == 1u);
    assert(psbt_validate_envelope(invalid_psbt, sizeof(invalid_psbt), &info) == PSBT_STATUS_BAD_MAGIC);
    assert(psbt_validate_envelope(truncated_psbt, sizeof(truncated_psbt), &info) == PSBT_STATUS_TRUNCATED);
    assert(psbt_validate_envelope(non_canonical_psbt, sizeof(non_canonical_psbt), &info) == PSBT_STATUS_NON_CANONICAL_SIZE);
    assert(psbt_validate_envelope(unterminated_psbt, sizeof(unterminated_psbt), &info) == PSBT_STATUS_MISSING_GLOBAL_END);
    assert(psbt_validate_envelope(0, 0u, &info) == PSBT_STATUS_EMPTY);
    assert(psbt_validate_envelope(valid_psbt, PSBT_MAX_BYTES + 1u, &info) == PSBT_STATUS_TOO_LARGE);
    return 0;
}