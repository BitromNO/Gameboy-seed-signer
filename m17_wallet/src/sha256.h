#ifndef SHA256_H
#define SHA256_H

#include <stddef.h>
#include <stdint.h>

void sha256_digest(const uint8_t *data, size_t length, uint8_t digest[32]);

#endif