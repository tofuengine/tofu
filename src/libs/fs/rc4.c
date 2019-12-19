#include "rc4.h"

extern void rc4_setup(rc4_context_t *ctx, const uint8_t *key, size_t key_length)
{
    size_t j = 0, k = 0;
    ctx->x = 0;
    ctx->y = 0;
    uint8_t *m = ctx->m;
    for (int i = 0; i < 256; ++i) {
        m[i] = i;
    }
    for (int i = 0; i < 256; ++i) {
        uint8_t a = m[i];
        j = (unsigned char)(j + a + key[k]);
        m[i] = m[j]; 
        m[j] = a;
        if (++k >= key_length) {
            k = 0;
        }
    }
}

extern void rc4_crypt(rc4_context_t *ctx, const uint8_t *src, uint8_t *dst, size_t src_length)
{ 
    uint8_t x = ctx->x;
    uint8_t y = ctx->y;
    uint8_t *m = ctx->m;
    for (size_t i = 0; i < src_length; ++i) {
        uint8_t a = m[++x], b;
        y += a;
        m[x] = b = m[y];
        m[y] = a;
        dst[i] =  src[i] ^ m[(uint8_t)(a + b)];
    }
    ctx->x = x;
    ctx->y = y;
}