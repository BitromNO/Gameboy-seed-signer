#include <assert.h>
#include <stdint.h>

#include "psbt_inspector.h"

int main(void) {
    static const uint8_t valid_psbt[] = { 0x70u, 0x73u, 0x62u, 0x74u, 0xFFu, 0x00u };
    static const uint8_t invalid_psbt[] = { 0x70u, 0x73u, 0x62u, 0x74u, 0x00u };
    PsbtFileInfo info = { 0u };

    assert(psbt_validate_envelope(valid_psbt, sizeof(valid_psbt), &info) == PSBT_STATUS_OK);
    assert(info.byte_count == sizeof(valid_psbt));
    assert(psbt_validate_envelope(invalid_psbt, sizeof(invalid_psbt), &info) == PSBT_STATUS_BAD_MAGIC);
    assert(psbt_validate_envelope(0, 0u, &info) == PSBT_STATUS_EMPTY);
    assert(psbt_validate_envelope(valid_psbt, PSBT_MAX_BYTES + 1u, &info) == PSBT_STATUS_TOO_LARGE);
    return 0;
}