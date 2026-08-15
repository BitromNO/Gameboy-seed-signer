#include "psbt_inspector.h"

static const uint8_t psbt_magic[] = { 0x70u, 0x73u, 0x62u, 0x74u, 0xFFu };

PsbtStatus psbt_validate_envelope(const uint8_t *data, size_t length, PsbtFileInfo *info) {
    size_t index;

    if (length == 0u) return PSBT_STATUS_EMPTY;
    if (length > PSBT_MAX_BYTES) return PSBT_STATUS_TOO_LARGE;
    if (data == 0 || length < sizeof(psbt_magic)) return PSBT_STATUS_BAD_MAGIC;
    for (index = 0u; index < sizeof(psbt_magic); index++) {
        if (data[index] != psbt_magic[index]) return PSBT_STATUS_BAD_MAGIC;
    }
    if (info != 0) info->byte_count = (uint32_t)length;
    return PSBT_STATUS_OK;
}

const char *psbt_status_message(PsbtStatus status) {
    switch (status) {
        case PSBT_STATUS_OK: return "PSBT envelope accepted";
        case PSBT_STATUS_EMPTY: return "PSBT file is empty";
        case PSBT_STATUS_TOO_LARGE: return "PSBT file exceeds development limit";
        case PSBT_STATUS_BAD_MAGIC: return "PSBT magic prefix is invalid";
    }
    return "Unknown PSBT status";
}