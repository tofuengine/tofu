#ifndef LIBXMP_IT_SEX_H
#define LIBXMP_IT_SEX_H

#include "hio.h"

extern int itsex_decompress8(HIO_HANDLE *src, uint8_t *dst, uint32_t len, int it215);
extern int itsex_decompress16(HIO_HANDLE *src, int16_t *dst, uint32_t len, int it215);

#endif
