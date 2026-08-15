#ifndef REVIEW_FLAGS_H
#define REVIEW_FLAGS_H

#include <stdint.h>

#include "psbt_review.h"

#define REVIEW_FLAG_LOCKTIME 0x01u
#define REVIEW_FLAG_NONFINAL_SEQUENCE 0x02u
#define REVIEW_FLAG_UNKNOWN_OUTPUT 0x04u
#define REVIEW_FLAG_FEE_UNAVAILABLE 0x08u
#define REVIEW_FLAG_FEE_LIMIT 0x10u

uint32_t psbt_review_flags(const PsbtReview *review, uint64_t maximum_fee_sats);

#endif