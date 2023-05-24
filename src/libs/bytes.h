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

#ifndef TOFU_LIBS_BYTES_H
#define TOFU_LIBS_BYTES_H

#include <stdint.h>

extern uint16_t bytes_swap16(uint16_t value);
extern uint32_t bytes_swap32(uint32_t value);
extern uint64_t bytes_swap64(uint64_t value);

extern int16_t bytes_i16le(int16_t i16);
extern uint16_t bytes_ui16le(uint16_t ui16);
extern int16_t bytes_i16be(int16_t i16);
extern uint16_t bytes_ui16be(uint16_t ui16);
extern int32_t bytes_i32le(int32_t i32);
extern uint32_t bytes_ui32le(uint32_t ui32);
extern int32_t bytes_i32be(int32_t i32);
extern uint32_t bytes_ui32be(uint32_t ui32);

#endif  /* TOFU_LIBS_BYTES_H */
