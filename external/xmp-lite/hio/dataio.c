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

#include "dataio.h"
#include <byteswap.h>

#if 0
static inline uint16 _reverse16(uint16 value)
{
    return (((value & 0xFFFF) << 8) |
            ((value & 0xFF00) >> 8));
}

static inline uint32 _reverse32(uint32 value) 
{
    return (((value & 0x000000FF) << 24) |
            ((value & 0x0000FF00) <<  8) |
            ((value & 0x00FF0000) >>  8) |
            ((value & 0xFF000000) >> 24));
}
#endif

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
