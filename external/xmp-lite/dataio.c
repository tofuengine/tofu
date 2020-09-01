/* Extended Module Player
 * Copyright (C) 1996-2018 Claudio Matsuoka and Hipolito Carraro Jr
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <byteswap.h>
#include <errno.h>
#include "common.h"

uint8 read8(FILE *f, int *err)
{
	uint8 b;
	size_t read = fread(&b, sizeof(uint8), 1, f);
	if (read != 1) {
		*err = ferror(f) ? errno : EOF;
		return 0xFF;
	}
	*err = 0;
	return b;
}

int8 read8s(FILE *f, int *err)
{
	int8 b;
	size_t read = fread(&b, sizeof(int8), 1, f);
	if (read != 1) {
		*err = ferror(f) ? errno : EOF;
		return 0;
	}
	*err = 0;
	return b;
}

uint16 read16l(FILE *f, int *err)
{
	uint16 a;
	size_t read = fread(&a, sizeof(uint16), 1, f);
	if (read != 1) {
		*err = ferror(f) ? errno : EOF;
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

uint16 read16b(FILE *f, int *err)
{
	uint16 a;
	size_t read = fread(&a, sizeof(uint16), 1, f);
	if (read != 1) {
		*err = ferror(f) ? errno : EOF;
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

uint32 read32l(FILE *f, int *err)
{
	uint32 a;
	size_t read = fread(&a, sizeof(uint32), 1, f);
	if (read != 1) {
		*err = ferror(f) ? errno : EOF;
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

uint32 read32b(FILE *f, int *err)
{
	uint32 a;
	size_t read = fread(&a, sizeof(uint32), 1, f);
	if (read != 1) {
		*err = ferror(f) ? errno : EOF;
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

uint16 readmem16l(const uint8 *m)
{
	uint16 a = *((uint16 *)m);
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return a;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	return bswap_16(a);
#else
#    error unsupported endianness
#endif
}

uint16 readmem16b(const uint8 *m)
{
	uint16 a = bswap_16(*((uint16 *)m));
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	return bswap_16(a);
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return a;
#else
#    error unsupported endianness
#endif
}

uint32 readmem32l(const uint8 *m)
{
	uint32 a = *((uint32 *)m);
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return a;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	return bswap_32(a);
#else
#    error unsupported endianness
#endif
}

uint32 readmem32b(const uint8 *m)
{
	uint32 a = bswap_32(*((uint32 *)m));
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	return bswap_32(a);
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return a;
#else
#    error unsupported endianness
#endif
}
