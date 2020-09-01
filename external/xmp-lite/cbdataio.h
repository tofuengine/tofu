#ifndef LIBXMP_CBDATAIO_H
#define LIBXMP_CBDATAIO_H

#include <byteswap.h>
#include <stddef.h>
#include <stdint.h>
#include "common.h"
#include "cbio.h"

static uint8 cbread8(CBFILE *cb, int *err)
{
	uint8 b;
	size_t read = cb->func.read(cb->ud, &b, sizeof(uint8));
	if (read != sizeof(uint8)) {
		*err = EOF;
		return 0xFF;
	}
	*err = 0;
	return b;
}

static int8 cbread8s(CBFILE *cb, int *err)
{
	int8 b;
	size_t read = cb->func.read(cb->ud, &b, sizeof(int8));
	if (read != sizeof(int8)) {
		*err = EOF;
		return 0;
	}
	*err = 0;
	return b;
}

static uint16 cbread16l(CBFILE *cb, int *err)
{
	uint16 a;
	size_t read = cb->func.read(cb->ud, &a, sizeof(uint16));
	if (read != sizeof(uint16)) {
		*err = EOF;
		return 0xffff;
	}
	*err = 0;
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return a;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	return bswap_16(a);
#else
#    error unsupported endianness
#endif
}

static uint16 cbread16b(CBFILE *cb, int *err)
{
	uint16 a;
	size_t read = cb->func.read(cb->ud, &a, sizeof(uint16));
	if (read != sizeof(uint16)) {
		*err = EOF;
		return 0xffff;
	}
	*err = 0;
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	return bswap_16(a);
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return a;
#else
#    error unsupported endianness
#endif
}

static uint32 cbread32l(CBFILE *cb, int *err)
{
	uint32 a;
	size_t read = cb->func.read(cb->ud, &a, sizeof(uint32));
	if (read != sizeof(uint32)) {
		*err = EOF;
		return 0xffffffff;
	}
	*err = 0;
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return a;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	return bswap_32(a);
#else
#    error unsupported endianness
#endif
}

static uint32 cbread32b(CBFILE *cb, int *err)
{
	uint32 a;
	size_t read = cb->func.read(cb->ud, &a, sizeof(uint32));
	if (read != sizeof(uint32)) {
		*err = EOF;
		return 0xffffffff;
	}
	*err = 0;
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	return bswap_32(a);
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return a;
#else
#    error unsupported endianness
#endif
}

#endif
