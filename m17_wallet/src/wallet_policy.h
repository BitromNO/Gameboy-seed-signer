#ifndef WALLET_POLICY_H
#define WALLET_POLICY_H

#include <stddef.h>
#include <stdint.h>

#include "psbt_review.h"

#define WALLET_POLICY_MAX_SCRIPTS 128u
#define WALLET_POLICY_MAX_SCRIPT_BYTES 40u

typedef struct WalletOwnedScript {
    uint8_t length;
    uint8_t bytes[WALLET_POLICY_MAX_SCRIPT_BYTES];
} WalletOwnedScript;

typedef struct WalletPolicy {
    uint16_t script_count;
    WalletOwnedScript scripts[WALLET_POLICY_MAX_SCRIPTS];
} WalletPolicy;

int wallet_policy_add_script(WalletPolicy *policy, const uint8_t *script, size_t length);
void wallet_policy_mark_change(const WalletPolicy *policy, PsbtReview *review);

#endif