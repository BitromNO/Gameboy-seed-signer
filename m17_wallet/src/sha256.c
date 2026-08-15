#include "sha256.h"

static uint32_t rotr32(uint32_t value, uint8_t count) {
    return (value >> count) | (value << (32u - count));
}

static uint32_t read_u32_be(const uint8_t *data) {
    return ((uint32_t)data[0] << 24) | ((uint32_t)data[1] << 16) | ((uint32_t)data[2] << 8) | data[3];
}

static void write_u32_be(uint8_t *data, uint32_t value) {
    data[0] = (uint8_t)(value >> 24);
    data[1] = (uint8_t)(value >> 16);
    data[2] = (uint8_t)(value >> 8);
    data[3] = (uint8_t)value;
}

static void sha256_compress(uint32_t state[8], const uint8_t block[64]) {
    static const uint32_t constants[64] = {
        0x428A2F98u,0x71374491u,0xB5C0FBCFu,0xE9B5DBA5u,0x3956C25Bu,0x59F111F1u,0x923F82A4u,0xAB1C5ED5u,
        0xD807AA98u,0x12835B01u,0x243185BEu,0x550C7DC3u,0x72BE5D74u,0x80DEB1FEu,0x9BDC06A7u,0xC19BF174u,
        0xE49B69C1u,0xEFBE4786u,0x0FC19DC6u,0x240CA1CCu,0x2DE92C6Fu,0x4A7484AAu,0x5CB0A9DCu,0x76F988DAu,
        0x983E5152u,0xA831C66Du,0xB00327C8u,0xBF597FC7u,0xC6E00BF3u,0xD5A79147u,0x06CA6351u,0x14292967u,
        0x27B70A85u,0x2E1B2138u,0x4D2C6DFCu,0x53380D13u,0x650A7354u,0x766A0ABBu,0x81C2C92Eu,0x92722C85u,
        0xA2BFE8A1u,0xA81A664Bu,0xC24B8B70u,0xC76C51A3u,0xD192E819u,0xD6990624u,0xF40E3585u,0x106AA070u,
        0x19A4C116u,0x1E376C08u,0x2748774Cu,0x34B0BCB5u,0x391C0CB3u,0x4ED8AA4Au,0x5B9CCA4Fu,0x682E6FF3u,
        0x748F82EEu,0x78A5636Fu,0x84C87814u,0x8CC70208u,0x90BEFFFAu,0xA4506CEBu,0xBEF9A3F7u,0xC67178F2u
    };
    uint32_t words[64];
    uint32_t working[8];
    uint32_t next_one;
    uint32_t next_two;
    uint32_t choice;
    uint32_t majority;
    uint8_t index;

    for (index = 0u; index < 16u; index++) words[index] = read_u32_be(block + (size_t)index * 4u);
    for (index = 16u; index < 64u; index++) words[index] = (rotr32(words[index - 2u], 17u) ^ rotr32(words[index - 2u], 19u) ^ (words[index - 2u] >> 10)) + words[index - 7u] + (rotr32(words[index - 15u], 7u) ^ rotr32(words[index - 15u], 18u) ^ (words[index - 15u] >> 3)) + words[index - 16u];
    for (index = 0u; index < 8u; index++) working[index] = state[index];
    for (index = 0u; index < 64u; index++) {
        choice = (working[4] & working[5]) ^ (~working[4] & working[6]);
        majority = (working[0] & working[1]) ^ (working[0] & working[2]) ^ (working[1] & working[2]);
        next_one = working[7] + (rotr32(working[4], 6u) ^ rotr32(working[4], 11u) ^ rotr32(working[4], 25u)) + choice + constants[index] + words[index];
        next_two = (rotr32(working[0], 2u) ^ rotr32(working[0], 13u) ^ rotr32(working[0], 22u)) + majority;
        working[7] = working[6]; working[6] = working[5]; working[5] = working[4]; working[4] = working[3] + next_one;
        working[3] = working[2]; working[2] = working[1]; working[1] = working[0]; working[0] = next_one + next_two;
    }
    for (index = 0u; index < 8u; index++) state[index] += working[index];
}

void sha256_digest(const uint8_t *data, size_t length, uint8_t digest[32]) {
    uint32_t state[8] = { 0x6A09E667u, 0xBB67AE85u, 0x3C6EF372u, 0xA54FF53Au, 0x510E527Fu, 0x9B05688Cu, 0x1F83D9ABu, 0x5BE0CD19u };
    uint8_t block[64];
    uint64_t bit_length = (uint64_t)length * 8u;
    size_t full_blocks = length / 64u;
    size_t tail_length = length % 64u;
    size_t index;
    uint8_t word_index;

    for (index = 0u; index < full_blocks; index++) sha256_compress(state, data + index * 64u);
    for (index = 0u; index < 64u; index++) block[index] = 0u;
    for (index = 0u; index < tail_length; index++) block[index] = data[full_blocks * 64u + index];
    block[tail_length] = 0x80u;
    if (tail_length >= 56u) {
        sha256_compress(state, block);
        for (index = 0u; index < 64u; index++) block[index] = 0u;
    }
    for (index = 0u; index < 8u; index++) block[63u - index] = (uint8_t)(bit_length >> (index * 8u));
    sha256_compress(state, block);
    for (word_index = 0u; word_index < 8u; word_index++) write_u32_be(digest + (size_t)word_index * 4u, state[word_index]);
}