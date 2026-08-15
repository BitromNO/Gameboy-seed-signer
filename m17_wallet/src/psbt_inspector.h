#ifndef PSBT_INSPECTOR_H
#define PSBT_INSPECTOR_H

#include <stddef.h>
#include <stdint.h>

typedef enum PsbtStatus {
    PSBT_STATUS_OK,
    PSBT_STATUS_EMPTY,
    PSBT_STATUS_TOO_LARGE,
    PSBT_STATUS_BAD_MAGIC,
    PSBT_STATUS_TRUNCATED,
    PSBT_STATUS_NON_CANONICAL_SIZE,
    PSBT_STATUS_EMPTY_KEY,
    PSBT_STATUS_MISSING_GLOBAL_END
} PsbtStatus;

typedef struct PsbtFileInfo {
    uint32_t byte_count;
    uint32_t global_map_bytes;
    uint16_t global_record_count;
} PsbtFileInfo;

#define PSBT_MAX_BYTES (1024u * 1024u)

PsbtStatus psbt_validate_envelope(const uint8_t *data, size_t length, PsbtFileInfo *info);
const char *psbt_status_message(PsbtStatus status);

#endif