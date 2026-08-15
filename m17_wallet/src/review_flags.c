#include "review_flags.h"

uint32_t psbt_review_flags(const PsbtReview *review, uint64_t maximum_fee_sats) {
    uint32_t flags = 0u;
    uint16_t index;

    if (review == 0) return REVIEW_FLAG_FEE_UNAVAILABLE;
    if (review->locktime != 0u) flags |= REVIEW_FLAG_LOCKTIME;
    for (index = 0u; index < review->input_count; index++) {
        if (review->inputs[index].sequence != 0xFFFFFFFFu) flags |= REVIEW_FLAG_NONFINAL_SEQUENCE;
        if (review->inputs[index].sequence < 0xFFFFFFFEu) flags |= REVIEW_FLAG_RBF;
    }
    for (index = 0u; index < review->output_count; index++) {
        if (review->outputs[index].type == BITCOIN_OUTPUT_UNKNOWN) flags |= REVIEW_FLAG_UNKNOWN_OUTPUT;
    }
    if (!review->fee_is_known) flags |= REVIEW_FLAG_FEE_UNAVAILABLE;
    else if (maximum_fee_sats != 0u && review->fee_sats > maximum_fee_sats) flags |= REVIEW_FLAG_FEE_LIMIT;
    return flags;
}