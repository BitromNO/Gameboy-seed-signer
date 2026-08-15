#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "psbt_inspector.h"
#include "psbt_review.h"
#include "review_flags.h"
#include "sha256.h"
#include "wallet_policy.h"

static size_t build_review_fixture(uint8_t data[256]) {
    static const uint8_t program[] = { 0x75u, 0x1Eu, 0x76u, 0xE8u, 0x19u, 0x91u, 0x96u, 0xD4u, 0x54u, 0x94u, 0x1Cu, 0x45u, 0xD1u, 0xB3u, 0xA3u, 0x23u, 0xF1u, 0x43u, 0x3Bu, 0xD6u };
    size_t offset = 0u;
    uint8_t index;

    data[offset++] = 0x70u; data[offset++] = 0x73u; data[offset++] = 0x62u; data[offset++] = 0x74u; data[offset++] = 0xFFu;
    data[offset++] = 0x01u; data[offset++] = 0x00u; data[offset++] = 0x52u;
    data[offset++] = 0x01u; data[offset++] = 0x00u; data[offset++] = 0x00u; data[offset++] = 0x00u; data[offset++] = 0x01u;
    for (index = 0u; index < 32u; index++) data[offset++] = 0u;
    data[offset++] = 0x00u; data[offset++] = 0x00u; data[offset++] = 0x00u; data[offset++] = 0x00u;
    data[offset++] = 0x00u;
    data[offset++] = 0xFFu; data[offset++] = 0xFFu; data[offset++] = 0xFFu; data[offset++] = 0xFFu;
    data[offset++] = 0x01u;
    data[offset++] = 0xE8u; data[offset++] = 0x03u; data[offset++] = 0x00u; data[offset++] = 0x00u; data[offset++] = 0x00u; data[offset++] = 0x00u; data[offset++] = 0x00u; data[offset++] = 0x00u;
    data[offset++] = 0x16u; data[offset++] = 0x00u; data[offset++] = 0x14u;
    for (index = 0u; index < sizeof(program); index++) data[offset++] = program[index];
    data[offset++] = 0x00u; data[offset++] = 0x00u; data[offset++] = 0x00u; data[offset++] = 0x00u;
    data[offset++] = 0x00u;
    data[offset++] = 0x01u; data[offset++] = 0x01u; data[offset++] = 0x1Fu;
    data[offset++] = 0xD0u; data[offset++] = 0x07u; data[offset++] = 0x00u; data[offset++] = 0x00u; data[offset++] = 0x00u; data[offset++] = 0x00u; data[offset++] = 0x00u; data[offset++] = 0x00u;
    data[offset++] = 0x16u; data[offset++] = 0x00u; data[offset++] = 0x14u;
    for (index = 0u; index < sizeof(program); index++) data[offset++] = program[index];
    data[offset++] = 0x00u;
    data[offset++] = 0x00u;
    return offset;
}

static size_t build_non_witness_review_fixture(uint8_t data[256]) {
    uint8_t transaction[256];
    size_t transaction_psbt_length = build_review_fixture(transaction);
    size_t transaction_length = 0x52u;
    size_t offset = 0u;

    data[offset++] = 0x70u; data[offset++] = 0x73u; data[offset++] = 0x62u; data[offset++] = 0x74u; data[offset++] = 0xFFu;
    data[offset++] = 0x01u; data[offset++] = 0x00u; data[offset++] = 0x52u;
    memcpy(data + offset, transaction + 8u, transaction_length);
    offset += transaction_length;
    data[offset++] = 0x00u;
    data[offset++] = 0x01u; data[offset++] = 0x00u; data[offset++] = 0x52u;
    memcpy(data + offset, transaction + 8u, transaction_length);
    offset += transaction_length;
    data[offset++] = 0x00u;
    data[offset++] = 0x00u;
    assert(transaction_psbt_length > transaction_length);
    return offset;
}

static size_t build_v2_review_fixture(uint8_t data[256]) {
    static const uint8_t program[] = { 0x75u, 0x1Eu, 0x76u, 0xE8u, 0x19u, 0x91u, 0x96u, 0xD4u, 0x54u, 0x94u, 0x1Cu, 0x45u, 0xD1u, 0xB3u, 0xA3u, 0x23u, 0xF1u, 0x43u, 0x3Bu, 0xD6u };
    size_t offset = 0u;
    uint8_t index;

    data[offset++] = 0x70u; data[offset++] = 0x73u; data[offset++] = 0x62u; data[offset++] = 0x74u; data[offset++] = 0xFFu;
    data[offset++] = 0x01u; data[offset++] = 0xFBu; data[offset++] = 0x04u; data[offset++] = 0x02u; data[offset++] = 0x00u; data[offset++] = 0x00u; data[offset++] = 0x00u;
    data[offset++] = 0x01u; data[offset++] = 0x04u; data[offset++] = 0x01u; data[offset++] = 0x01u;
    data[offset++] = 0x01u; data[offset++] = 0x05u; data[offset++] = 0x01u; data[offset++] = 0x01u;
    data[offset++] = 0x00u;
    data[offset++] = 0x01u; data[offset++] = 0x0Eu; data[offset++] = 0x20u;
    for (index = 0u; index < 32u; index++) data[offset++] = 0u;
    data[offset++] = 0x01u; data[offset++] = 0x0Fu; data[offset++] = 0x04u;
    data[offset++] = 0x00u; data[offset++] = 0x00u; data[offset++] = 0x00u; data[offset++] = 0x00u;
    data[offset++] = 0x01u; data[offset++] = 0x01u; data[offset++] = 0x1Fu;
    data[offset++] = 0xD0u; data[offset++] = 0x07u; data[offset++] = 0x00u; data[offset++] = 0x00u; data[offset++] = 0x00u; data[offset++] = 0x00u; data[offset++] = 0x00u; data[offset++] = 0x00u;
    data[offset++] = 0x16u; data[offset++] = 0x00u; data[offset++] = 0x14u;
    for (index = 0u; index < sizeof(program); index++) data[offset++] = program[index];
    data[offset++] = 0x00u;
    data[offset++] = 0x01u; data[offset++] = 0x03u; data[offset++] = 0x08u;
    data[offset++] = 0xE8u; data[offset++] = 0x03u; data[offset++] = 0x00u; data[offset++] = 0x00u; data[offset++] = 0x00u; data[offset++] = 0x00u; data[offset++] = 0x00u; data[offset++] = 0x00u;
    data[offset++] = 0x01u; data[offset++] = 0x04u; data[offset++] = 0x16u; data[offset++] = 0x00u; data[offset++] = 0x14u;
    for (index = 0u; index < sizeof(program); index++) data[offset++] = program[index];
    data[offset++] = 0x00u;
    return offset;
}

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
    static const uint8_t valid_psbt_v2[] = {
        0x70u, 0x73u, 0x62u, 0x74u, 0xFFu,
        0x01u, 0xFBu, 0x04u, 0x02u, 0x00u, 0x00u, 0x00u,
        0x01u, 0x04u, 0x01u, 0x01u,
        0x01u, 0x05u, 0x01u, 0x01u,
        0x00u
    };
    static const uint8_t duplicate_version[] = { 0x70u, 0x73u, 0x62u, 0x74u, 0xFFu, 0x01u, 0xFBu, 0x04u, 0x02u, 0x00u, 0x00u, 0x00u, 0x01u, 0xFBu, 0x04u, 0x02u, 0x00u, 0x00u, 0x00u, 0x00u };
    static const uint8_t invalid_version[] = { 0x70u, 0x73u, 0x62u, 0x74u, 0xFFu, 0x01u, 0xFBu, 0x04u, 0x01u, 0x00u, 0x00u, 0x00u, 0x00u };
    static const uint8_t missing_v2_counts[] = { 0x70u, 0x73u, 0x62u, 0x74u, 0xFFu, 0x01u, 0xFBu, 0x04u, 0x02u, 0x00u, 0x00u, 0x00u, 0x00u };
    static const uint8_t invalid_psbt[] = { 0x70u, 0x73u, 0x62u, 0x74u, 0x00u };
    static const uint8_t truncated_psbt[] = { 0x70u, 0x73u, 0x62u, 0x74u, 0xFFu, 0x02u, 0x00u };
    static const uint8_t non_canonical_psbt[] = { 0x70u, 0x73u, 0x62u, 0x74u, 0xFFu, 0xFDu, 0x01u, 0x00u };
    static const uint8_t unterminated_psbt[] = { 0x70u, 0x73u, 0x62u, 0x74u, 0xFFu, 0x01u, 0xFCu, 0x00u };
    static const uint8_t p2pkh[] = { 0x76u, 0xA9u, 0x14u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0x88u, 0xACu };
    static const uint8_t bip173_p2pkh[] = { 0x76u, 0xA9u, 0x14u, 0x75u, 0x1Eu, 0x76u, 0xE8u, 0x19u, 0x91u, 0x96u, 0xD4u, 0x54u, 0x94u, 0x1Cu, 0x45u, 0xD1u, 0xB3u, 0xA3u, 0x23u, 0xF1u, 0x43u, 0x3Bu, 0xD6u, 0x88u, 0xACu };
    static const uint8_t p2wpkh[] = { 0x00u, 0x14u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u };
    static const uint8_t p2tr[] = { 0x51u, 0x20u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u };
    static const uint8_t op_return[] = { 0x6Au, 0x02u, 0x01u, 0x02u };
    static const uint8_t bip173_p2wpkh[] = { 0x00u, 0x14u, 0x75u, 0x1Eu, 0x76u, 0xE8u, 0x19u, 0x91u, 0x96u, 0xD4u, 0x54u, 0x94u, 0x1Cu, 0x45u, 0xD1u, 0xB3u, 0xA3u, 0x23u, 0xF1u, 0x43u, 0x3Bu, 0xD6u };
    static const uint8_t bip350_p2tr[] = { 0x51u, 0x20u, 0x79u, 0xBEu, 0x66u, 0x7Eu, 0xF9u, 0xDCu, 0xBBu, 0xACu, 0x55u, 0xA0u, 0x62u, 0x95u, 0xCEu, 0x87u, 0x0Bu, 0x07u, 0x02u, 0x9Bu, 0xFCu, 0xDBu, 0x2Du, 0xCEu, 0x28u, 0xD9u, 0x59u, 0xF2u, 0x81u, 0x5Bu, 0x16u, 0xF8u, 0x17u, 0x98u };
    PsbtFileInfo info = { 0u, 0u, 0u, PSBT_VERSION_UNKNOWN, 0u, 0u, 0u, 0u };
    char address[91];
    uint8_t digest[32];
    uint8_t review_fixture[256];
    uint8_t non_witness_review_fixture[256];
    uint8_t v2_review_fixture[256];
    PsbtReview review;
    WalletPolicy policy = { 0u, { { 0u, { 0u } } } };
    size_t review_length;
    size_t non_witness_review_length;
    size_t v2_review_length;
    static const uint8_t sha256_abc[32] = { 0xBAu, 0x78u, 0x16u, 0xBFu, 0x8Fu, 0x01u, 0xCFu, 0xEAu, 0x41u, 0x41u, 0x40u, 0xDEu, 0x5Du, 0xAEu, 0x22u, 0x23u, 0xB0u, 0x03u, 0x61u, 0xA3u, 0x96u, 0x17u, 0x7Au, 0x9Cu, 0xB4u, 0x10u, 0xFFu, 0x61u, 0xF2u, 0x00u, 0x15u, 0xADu };

    assert(psbt_validate_envelope(valid_psbt_v0, sizeof(valid_psbt_v0), &info) == PSBT_STATUS_OK);
    assert(info.byte_count == sizeof(valid_psbt_v0));
    assert(info.global_map_bytes == 64u);
    assert(info.global_record_count == 1u);
    assert(info.version == PSBT_VERSION_V0);
    assert(info.input_count == 1u);
    assert(info.output_count == 1u);
    assert(info.recognized_output_count == 0u);
    assert(info.total_output_sats == 1000u);
    assert(bitcoin_classify_output_script(p2pkh, sizeof(p2pkh)) == BITCOIN_OUTPUT_P2PKH);
    assert(bitcoin_classify_output_script(p2wpkh, sizeof(p2wpkh)) == BITCOIN_OUTPUT_P2WPKH);
    assert(bitcoin_classify_output_script(p2tr, sizeof(p2tr)) == BITCOIN_OUTPUT_P2TR);
    assert(bitcoin_classify_output_script(op_return, sizeof(op_return)) == BITCOIN_OUTPUT_OP_RETURN);
    assert(bitcoin_encode_mainnet_segwit_address(bip173_p2wpkh, sizeof(bip173_p2wpkh), address, sizeof(address)) == 1);
    assert(strcmp(address, "bc1qw508d6qejxtdg4y5r3zarvary0c5xw7kv8f3t4") == 0);
    assert(bitcoin_encode_mainnet_segwit_address(bip350_p2tr, sizeof(bip350_p2tr), address, sizeof(address)) == 1);
    assert(strcmp(address, "bc1p0xlxvlhemja6c4dqv22uapctqupfhlxm9h8z3k2e72q4k9hcz7vqzk5jj0") == 0);
    assert(bitcoin_encode_mainnet_legacy_address(bip173_p2pkh, sizeof(bip173_p2pkh), address, sizeof(address)) == 1);
    assert(strcmp(address, "1BgGZ9tcN4rm9KBzDn7KprQz87SZ26SAMH") == 0);
    sha256_digest((const uint8_t *)"abc", 3u, digest);
    assert(memcmp(digest, sha256_abc, sizeof(digest)) == 0);
    review_length = build_review_fixture(review_fixture);
    assert(psbt_parse_v0_review(review_fixture, review_length, &review) == PSBT_REVIEW_OK);
    assert(review.input_count == 1u);
    assert(review.output_count == 1u);
    assert(review.transaction_version == 1u);
    assert(review.locktime == 0u);
    assert(review.inputs[0].previous_output_index == 0u);
    assert(review.inputs[0].sequence == 0xFFFFFFFFu);
    assert(review.known_input_amount_count == 1u);
    assert(review.total_input_sats == 2000u);
    assert(review.total_output_sats == 1000u);
    assert(review.fee_is_known == 1u);
    assert(review.fee_sats == 1000u);
    assert(review.outputs[0].type == BITCOIN_OUTPUT_P2WPKH);
    assert(psbt_review_flags(&review, 0u) == 0u);
    review.locktime = 500u;
    review.inputs[0].sequence = 0xFFFFFFFEu;
    review.outputs[0].type = BITCOIN_OUTPUT_UNKNOWN;
    assert(psbt_review_flags(&review, 500u) == (REVIEW_FLAG_LOCKTIME | REVIEW_FLAG_NONFINAL_SEQUENCE | REVIEW_FLAG_UNKNOWN_OUTPUT | REVIEW_FLAG_FEE_LIMIT));
    assert(strcmp(review.outputs[0].address, "bc1qw508d6qejxtdg4y5r3zarvary0c5xw7kv8f3t4") == 0);
    assert(wallet_policy_add_script(&policy, bip173_p2wpkh, sizeof(bip173_p2wpkh)) == 1);
    wallet_policy_mark_change(&policy, &review);
    assert(review.outputs[0].is_change == 1u);
    policy.scripts[0].bytes[2] ^= 1u;
    wallet_policy_mark_change(&policy, &review);
    assert(review.outputs[0].is_change == 0u);
    non_witness_review_length = build_non_witness_review_fixture(non_witness_review_fixture);
    assert(psbt_parse_v0_review(non_witness_review_fixture, non_witness_review_length, &review) == PSBT_REVIEW_OK);
    assert(review.known_input_amount_count == 1u);
    assert(review.total_input_sats == 1000u);
    assert(review.fee_is_known == 1u);
    assert(review.fee_sats == 0u);
    v2_review_length = build_v2_review_fixture(v2_review_fixture);
    assert(psbt_parse_v2_review(v2_review_fixture, v2_review_length, &review) == PSBT_REVIEW_OK);
    assert(review.version == PSBT_VERSION_V2);
    assert(review.input_count == 1u);
    assert(review.output_count == 1u);
    assert(review.total_output_sats == 1000u);
    assert(review.total_input_sats == 2000u);
    assert(review.fee_is_known == 1u);
    assert(review.fee_sats == 1000u);
    assert(strcmp(review.outputs[0].address, "bc1qw508d6qejxtdg4y5r3zarvary0c5xw7kv8f3t4") == 0);
    assert(psbt_validate_envelope(valid_psbt_v2, sizeof(valid_psbt_v2), &info) == PSBT_STATUS_OK);
    assert(info.version == PSBT_VERSION_V2);
    assert(info.input_count == 1u);
    assert(info.output_count == 1u);
    assert(psbt_validate_envelope(duplicate_version, sizeof(duplicate_version), &info) == PSBT_STATUS_DUPLICATE_GLOBAL_FIELD);
    assert(psbt_validate_envelope(invalid_version, sizeof(invalid_version), &info) == PSBT_STATUS_INVALID_GLOBAL_VERSION);
    assert(psbt_validate_envelope(missing_v2_counts, sizeof(missing_v2_counts), &info) == PSBT_STATUS_MISSING_V2_COUNTS);
    assert(psbt_validate_envelope(invalid_psbt, sizeof(invalid_psbt), &info) == PSBT_STATUS_BAD_MAGIC);
    assert(psbt_validate_envelope(truncated_psbt, sizeof(truncated_psbt), &info) == PSBT_STATUS_TRUNCATED);
    assert(psbt_validate_envelope(non_canonical_psbt, sizeof(non_canonical_psbt), &info) == PSBT_STATUS_NON_CANONICAL_SIZE);
    assert(psbt_validate_envelope(unterminated_psbt, sizeof(unterminated_psbt), &info) == PSBT_STATUS_MISSING_GLOBAL_END);
    assert(psbt_validate_envelope(0, 0u, &info) == PSBT_STATUS_EMPTY);
    assert(psbt_validate_envelope(valid_psbt_v0, PSBT_MAX_BYTES + 1u, &info) == PSBT_STATUS_TOO_LARGE);
    return 0;
}