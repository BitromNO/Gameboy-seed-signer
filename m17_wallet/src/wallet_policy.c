#include "wallet_policy.h"

#include <string.h>

int wallet_policy_add_script(WalletPolicy *policy, const uint8_t *script, size_t length) {
    WalletOwnedScript *destination;

    if (policy == 0 || script == 0 || length == 0u || length > WALLET_POLICY_MAX_SCRIPT_BYTES || policy->script_count == WALLET_POLICY_MAX_SCRIPTS) return 0;
    destination = &policy->scripts[policy->script_count++];
    destination->length = (uint8_t)length;
    memcpy(destination->bytes, script, length);
    return 1;
}

void wallet_policy_mark_change(const WalletPolicy *policy, PsbtReview *review) {
    uint16_t output_index;
    uint16_t script_index;

    if (policy == 0 || review == 0) return;
    for (output_index = 0u; output_index < review->output_count; output_index++) {
        review->outputs[output_index].is_change = 0u;
        for (script_index = 0u; script_index < policy->script_count; script_index++) {
            if (review->outputs[output_index].script_length == policy->scripts[script_index].length &&
                memcmp(review->outputs[output_index].script, policy->scripts[script_index].bytes, policy->scripts[script_index].length) == 0) {
                review->outputs[output_index].is_change = 1u;
                break;
            }
        }
    }
}