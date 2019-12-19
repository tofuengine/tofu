#ifndef __RC4_H__
#define __RC4_H__

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint8_t i, j;
    uint8_t S[256];
} rc4_context_t;

extern void rc4_schedule(rc4_context_t *context, const uint8_t *key, size_t key_size);
extern void rc4_process(rc4_context_t *context, uint8_t *data, size_t data_size);

#endif  /* __RC4_H__ */
