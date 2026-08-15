#ifndef PSBT_REVIEW_H
#define PSBT_REVIEW_H

#include <stddef.h>
#include <stdint.h>

#include "psbt_inspector.h"

#define PSBT_REVIEW_MAX_INPUTS 64u
#define PSBT_REVIEW_MAX_OUTPUTS 64u
#define PSBT_REVIEW_ADDRESS_MAX 91u

typedef enum PsbtReviewStatus {
    PSBT_REVIEW_OK,
    PSBT_REVIEW_ENVELOPE_REJECTED,
    PSBT_REVIEW_UNSUPPORTED_VERSION,
    PSBT_REVIEW_TRANSACTION_TRUNCATED,
    PSBT_REVIEW_TRANSACTION_LIMIT,
    PSBT_REVIEW_MAP_TRUNCATED,
    PSBT_REVIEW_DUPLICATE_FIELD,
    PSBT_REVIEW_UNSUPPORTED_INPUT_AMOUNT
} PsbtReviewStatus;

typedef struct PsbtOutputReview {
    uint64_t amount_sats;
    BitcoinOutputType type;
    uint8_t script_length;
    uint8_t script[40];
    uint8_t is_change;
    char address[PSBT_REVIEW_ADDRESS_MAX];
} PsbtOutputReview;

typedef struct PsbtInputReview {
    uint8_t previous_transaction_id[32];
    uint32_t previous_output_index;
    uint32_t sequence;
    uint8_t amount_is_known;
    uint64_t amount_sats;
} PsbtInputReview;

typedef struct PsbtReview {
    PsbtVersion version;
    uint32_t transaction_version;
    uint32_t locktime;
    uint16_t input_count;
    uint16_t output_count;
    uint16_t known_input_amount_count;
    uint64_t total_input_sats;
    uint64_t total_output_sats;
    uint64_t fee_sats;
    uint8_t fee_is_known;
    PsbtInputReview inputs[PSBT_REVIEW_MAX_INPUTS];
    PsbtOutputReview outputs[PSBT_REVIEW_MAX_OUTPUTS];
} PsbtReview;

PsbtReviewStatus psbt_parse_v0_review(const uint8_t *data, size_t length, PsbtReview *review);
PsbtReviewStatus psbt_parse_v2_review(const uint8_t *data, size_t length, PsbtReview *review);
const char *psbt_review_status_message(PsbtReviewStatus status);

#endif