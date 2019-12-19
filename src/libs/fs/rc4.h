#ifndef __RC4_H__
#define __RC4_H__

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint8_t x, y, m[256];
} rc4_context_t;

extern void rc4_setup(rc4_context_t *ctx, const uint8_t *key, size_t key_length);
extern void rc4_crypt(rc4_context_t *ctx, const uint8_t *src, uint8_t *dst, size_t src_length);

#endif  /* __RC4_H__ */
