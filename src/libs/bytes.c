/*
 * MIT License
 * 
 * Copyright (c) 2019-2023 Marco Lizza
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "bytes.h"

// Can't use `byteswap.h` because it's not available on Windows.
uint16_t bytes_swap16(uint16_t value)
{
    return (((value & 0x00FF) << 8)
            | ((value & 0xFF00) >> 8));
}


uint32_t bytes_swap32(uint32_t value) 
{
    return (((value & 0x000000FF) << 24)
            | ((value & 0x0000FF00) << 8)
            | ((value & 0x00FF0000) >> 8)
            | ((value & 0xFF000000) >> 24));
}

uint64_t bytes_swap64(uint64_t value) 
{
    return (((value & 0xFF00000000000000) >> 56)
            | ((value & 0x00FF000000000000) >> 40)
            | ((value & 0x0000FF0000000000) >> 24)
            | ((value & 0x000000FF00000000) >> 8)
            | ((value & 0x00000000FF000000) << 8)
            | ((value & 0x0000000000FF0000) << 24)
            | ((value & 0x000000000000FF00) << 40)
            | ((value & 0x00000000000000FF) << 56));
}

int16_t bytes_i16le(int16_t i16)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	return i16;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return bytes_swap16(i16);
#else
#    error unsupported endianness
#endif
}

uint16_t bytes_ui16le(uint16_t ui16)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	return ui16;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return bytes_swap16(ui16);
#else
#    error unsupported endianness
#endif
}

int16_t bytes_i16be(int16_t i16)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return bytes_swap16(i16);
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	return i16;
#else
#    error unsupported endianness
#endif
}

uint16_t bytes_ui16be(uint16_t ui16)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return bytes_swap16(ui16);
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	return ui16;
#else
#    error unsupported endianness
#endif
}

int32_t bytes_i32le(int32_t i32)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	return i32;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return bytes_swap32(i32);
#else
#    error unsupported endianness
#endif
}

uint32_t bytes_ui32le(uint32_t ui32)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	return ui32;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return bytes_swap32(ui32);
#else
#    error unsupported endianness
#endif
}

int32_t bytes_i32be(int32_t i32)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return bytes_swap32(i32);
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	return i32;
#else
#    error unsupported endianness
#endif
}

uint32_t bytes_ui32be(uint32_t ui32)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return bytes_swap32(ui32);
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	return ui32;
#else
#    error unsupported endianness
#endif
}
