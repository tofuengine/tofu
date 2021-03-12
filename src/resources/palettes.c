/*
 * MIT License
 * 
 * Copyright (c) 2019-2021 Marco Lizza
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

#include "palettes.h"

#include <strings.h>

typedef struct _Resource_Palette_t {
    const char *id;
    GL_Palette_t palette;
} Resource_Palette_t;

static const Resource_Palette_t _palettes[] = {
    { "gameboy", {
        {
            { .r =   8, .g =  24, .b =  32, .a = 255 },
            { .r =  52, .g = 104, .b =  86, .a = 255 },
            { .r = 136, .g = 192, .b = 112, .a = 255 },
            { .r = 224, .g = 248, .b = 208, .a = 255 }
        }, 4 }
    },
    { "gameboy-bw", {
        {
            { .r =   0, .g =   0, .b =   0, .a = 255 },
            { .r = 103, .g = 103, .b = 103, .a = 255 },
            { .r = 182, .g = 182, .b = 182, .a = 255 },
            { .r = 255, .g = 255, .b = 255, .a = 255 }
        }, 4 }
    },
    { "3-bit-rgb", {
        {
            { .r =   0, .g =   0, .b =   0, .a = 255 },
            { .r = 255, .g =   0, .b =   0, .a = 255 },
            { .r =   0, .g = 255, .b =   0, .a = 255 },
            { .r =   0, .g =   0, .b = 255, .a = 255 },
            { .r =   0, .g = 255, .b = 255, .a = 255 },
            { .r = 255, .g =   0, .b = 255, .a = 255 },
            { .r = 255, .g = 255, .b =   0, .a = 255 },
            { .r = 255, .g = 255, .b = 255, .a = 255 }
        }, 8 }
    },
    { "arne-8", {
        {
            { .r =   0, .g =   0, .b =   0, .a = 255 },
            { .r = 204, .g =  53, .b =   0, .a = 255 },
            { .r =  94, .g = 200, .b =   9, .a = 255 },
            { .r =  29, .g =  40, .b = 111, .a = 255 },
            { .r =   0, .g = 196, .b = 255, .a = 255 },
            { .r = 142, .g = 142, .b = 142, .a = 255 },
            { .r = 255, .g = 224, .b =  82, .a = 255 },
            { .r = 255, .g = 255, .b = 255, .a = 255 }
        }, 8 }
    },
    { "dawnbringer-8", {
        {
            { .r =   0, .g =   0, .b =   0, .a = 255 },
            { .r =  85, .g =  65, .b =  95, .a = 255 },
            { .r = 100, .g = 105, .b = 100, .a = 255 },
            { .r = 215, .g = 115, .b =  85, .a = 255 },
            { .r =  80, .g = 140, .b = 215, .a = 255 },
            { .r = 100, .g = 185, .b = 100, .a = 255 },
            { .r = 230, .g = 200, .b = 110, .a = 255 },
            { .r = 220, .g = 245, .b = 255, .a = 255 }
        }, 8 }
    },
    { "pico-8", {
        {
            { .r =   0, .g =   0, .b =   0, .a = 255 },
            { .r =  29, .g =  43, .b =  83, .a = 255 },
            { .r = 126, .g =  37, .b =  83, .a = 255 },
            { .r =   0, .g = 135, .b =  81, .a = 255 },
            { .r = 171, .g =  82, .b =  54, .a = 255 },
            { .r =  95, .g =  87, .b =  79, .a = 255 },
            { .r = 194, .g = 195, .b = 199, .a = 255 },
            { .r = 255, .g = 241, .b = 232, .a = 255 },
            { .r = 255, .g =   0, .b =  77, .a = 255 },
            { .r = 255, .g = 163, .b =   0, .a = 255 },
            { .r = 255, .g = 236, .b =  39, .a = 255 },
            { .r =   0, .g = 228, .b =  54, .a = 255 },
            { .r =  41, .g = 173, .b = 255, .a = 255 },
            { .r = 131, .g = 118, .b = 156, .a = 255 },
            { .r = 255, .g = 119, .b = 168, .a = 255 },
            { .r = 255, .g = 204, .b = 170, .a = 255 }
        }, 16 }
    },
    { "arne-16", {
        {
            { .r =   0, .g =   0, .b =   0, .a = 255 },
            { .r =  73, .g =  60, .b =  43, .a = 255 },
            { .r = 190, .g =  38, .b =  51, .a = 255 },
            { .r = 224, .g = 111, .b = 139, .a = 255 },
            { .r = 157, .g = 157, .b = 157, .a = 255 },
            { .r = 164, .g = 100, .b =  34, .a = 255 },
            { .r = 235, .g = 137, .b =  49, .a = 255 },
            { .r = 247, .g = 226, .b = 107, .a = 255 },
            { .r = 255, .g = 255, .b = 255, .a = 255 },
            { .r =  27, .g =  38, .b =  50, .a = 255 },
            { .r =  47, .g =  72, .b =  78, .a = 255 },
            { .r =  68, .g = 137, .b =  26, .a = 255 },
            { .r = 163, .g = 206, .b =  39, .a = 255 },
            { .r =   0, .g =  87, .b = 132, .a = 255 },
            { .r =  49, .g = 162, .b = 242, .a = 255 },
            { .r = 178, .g = 220, .b = 239, .a = 255 }
        }, 16 }
    },
    { "dawnbringer-16", {
        {
            { .r =  20, .g =  12, .b =  28, .a = 255 },
            { .r =  68, .g =  36, .b =  52, .a = 255 },
            { .r =  48, .g =  52, .b = 109, .a = 255 },
            { .r =  78, .g =  74, .b =  78, .a = 255 },
            { .r = 133, .g =  76, .b =  48, .a = 255 },
            { .r =  52, .g = 101, .b =  36, .a = 255 },
            { .r = 208, .g =  70, .b =  72, .a = 255 },
            { .r = 117, .g = 113, .b =  97, .a = 255 },
            { .r =  89, .g = 125, .b = 206, .a = 255 },
            { .r = 210, .g = 125, .b =  44, .a = 255 },
            { .r = 133, .g = 149, .b = 161, .a = 255 },
            { .r = 109, .g = 170, .b =  44, .a = 255 },
            { .r = 210, .g = 170, .b = 153, .a = 255 },
            { .r = 109, .g = 194, .b = 202, .a = 255 },
            { .r = 218, .g = 212, .b =  94, .a = 255 },
            { .r = 222, .g = 238, .b = 214, .a = 255 }
        }, 16 }
    },
    { "cga", {
        {
            { .r =   0, .g =   0, .b =   0, .a = 255 },
            { .r =  85, .g =  85, .b =  85, .a = 255 },
            { .r = 170, .g = 170, .b = 170, .a = 255 },
            { .r = 255, .g = 255, .b = 255, .a = 255 },
            { .r =   0, .g =   0, .b = 170, .a = 255 },
            { .r =  85, .g =  85, .b = 255, .a = 255 },
            { .r =   0, .g = 170, .b =   0, .a = 255 },
            { .r =  85, .g = 255, .b =  85, .a = 255 },
            { .r =   0, .g = 170, .b = 170, .a = 255 },
            { .r =  85, .g = 255, .b = 255, .a = 255 },
            { .r = 170, .g =   0, .b =   0, .a = 255 },
            { .r = 255, .g =  85, .b =  85, .a = 255 },
            { .r = 170, .g =   0, .b = 170, .a = 255 },
            { .r = 255, .g =  85, .b = 255, .a = 255 },
            { .r = 170, .g =  85, .b =   0, .a = 255 },
            { .r = 255, .g = 255, .b =  85, .a = 255 }
        }, 16 }
    },
    { "c64", {
        {
            { .r =   0, .g =   0, .b =   0, .a = 255 },
            { .r =  98, .g =  98, .b =  98, .a = 255 },
            { .r = 137, .g = 137, .b = 137, .a = 255 },
            { .r = 173, .g = 173, .b = 173, .a = 255 },
            { .r = 255, .g = 255, .b = 255, .a = 255 },
            { .r = 159, .g =  78, .b =  68, .a = 255 },
            { .r = 203, .g = 126, .b = 117, .a = 255 },
            { .r = 109, .g =  84, .b =  18, .a = 255 },
            { .r = 161, .g = 104, .b =  60, .a = 255 },
            { .r = 201, .g = 212, .b = 135, .a = 255 },
            { .r = 154, .g = 226, .b = 155, .a = 255 },
            { .r =  92, .g = 171, .b =  94, .a = 255 },
            { .r = 106, .g = 191, .b = 198, .a = 255 },
            { .r = 136, .g = 126, .b = 203, .a = 255 },
            { .r =  80, .g =  69, .b = 155, .a = 255 },
            { .r = 160, .g =  87, .b = 163, .a = 255 }
        }, 16 }
    },
    { "pico-8-ext", {
        {
            { .r =   0, .g =   0, .b =   0, .a = 255 },
            { .r =  29, .g =  43, .b =  83, .a = 255 },
            { .r = 126, .g =  37, .b =  83, .a = 255 },
            { .r =   0, .g = 135, .b =  81, .a = 255 },
            { .r = 171, .g =  82, .b =  54, .a = 255 },
            { .r =  95, .g =  87, .b =  79, .a = 255 },
            { .r = 194, .g = 195, .b = 199, .a = 255 },
            { .r = 255, .g = 241, .b = 232, .a = 255 },
            { .r = 255, .g =   0, .b =  77, .a = 255 },
            { .r = 255, .g = 163, .b =   0, .a = 255 },
            { .r = 255, .g = 236, .b =  39, .a = 255 },
            { .r =   0, .g = 228, .b =  54, .a = 255 },
            { .r =  41, .g = 173, .b = 255, .a = 255 },
            { .r = 131, .g = 118, .b = 156, .a = 255 },
            { .r = 255, .g = 119, .b = 168, .a = 255 },
            { .r = 255, .g = 204, .b = 170, .a = 255 },
            { .r =  41, .g =  24, .b =  20, .a = 255 },
            { .r =  17, .g =  29, .b =  53, .a = 255 },
            { .r =  66, .g =  33, .b =  54, .a = 255 },
            { .r =  18, .g =  83, .b =  89, .a = 255 },
            { .r = 116, .g =  47, .b =  41, .a = 255 },
            { .r =  73, .g =  51, .b =  59, .a = 255 },
            { .r = 162, .g = 136, .b = 121, .a = 255 },
            { .r = 243, .g = 239, .b = 125, .a = 255 },
            { .r = 190, .g =  18, .b =  80, .a = 255 },
            { .r = 255, .g = 108, .b =  36, .a = 255 },
            { .r = 168, .g = 231, .b =  46, .a = 255 },
            { .r =   0, .g = 181, .b =  67, .a = 255 },
            { .r =   6, .g =  90, .b = 181, .a = 255 },
            { .r = 117, .g =  70, .b = 101, .a = 255 },
            { .r = 255, .g = 110, .b =  89, .a = 255 },
            { .r = 255, .g = 157, .b = 129, .a = 255 }
        }, 32 }
    },
    { "arne-32", {
        {
            { .r =   0, .g =   0, .b =   0, .a = 255 },
            { .r =  73, .g =  60, .b =  43, .a = 255 },
            { .r = 190, .g =  38, .b =  51, .a = 255 },
            { .r = 224, .g = 111, .b = 139, .a = 255 },
            { .r = 164, .g = 100, .b =  34, .a = 255 },
            { .r = 235, .g = 137, .b =  49, .a = 255 },
            { .r = 247, .g = 226, .b = 107, .a = 255 },
            { .r = 255, .g = 255, .b = 255, .a = 255 },
            { .r = 157, .g = 157, .b = 157, .a = 255 },
            { .r =  47, .g =  72, .b =  78, .a = 255 },
            { .r =  27, .g =  38, .b =  50, .a = 255 },
            { .r =  68, .g = 137, .b =  26, .a = 255 },
            { .r = 163, .g = 206, .b =  39, .a = 255 },
            { .r =   0, .g =  87, .b = 132, .a = 255 },
            { .r =  49, .g = 162, .b = 242, .a = 255 },
            { .r = 178, .g = 220, .b = 239, .a = 255 },
            { .r =  52, .g =  42, .b = 151, .a = 255 },
            { .r = 101, .g = 109, .b = 113, .a = 255 },
            { .r = 204, .g = 204, .b = 204, .a = 255 },
            { .r = 115, .g =  41, .b =  48, .a = 255 },
            { .r = 203, .g =  67, .b = 167, .a = 255 },
            { .r =  82, .g =  79, .b =  64, .a = 255 },
            { .r = 173, .g = 157, .b =  51, .a = 255 },
            { .r = 236, .g =  71, .b =   0, .a = 255 },
            { .r = 250, .g = 180, .b =  11, .a = 255 },
            { .r =  17, .g =  94, .b =  51, .a = 255 },
            { .r =  20, .g = 128, .b = 126, .a = 255 },
            { .r =  21, .g = 194, .b = 165, .a = 255 },
            { .r =  34, .g =  90, .b = 246, .a = 255 },
            { .r = 153, .g = 100, .b = 249, .a = 255 },
            { .r = 247, .g = 142, .b = 214, .a = 255 },
            { .r = 244, .g = 185, .b = 144, .a = 255 }
        }, 32 }
    },
    { "dawnbringer-32", {
        {
            { .r =   0, .g =   0, .b =   0, .a = 255 },
            { .r =  34, .g =  32, .b =  52, .a = 255 },
            { .r =  69, .g =  40, .b =  60, .a = 255 },
            { .r = 102, .g =  57, .b =  49, .a = 255 },
            { .r = 143, .g =  86, .b =  59, .a = 255 },
            { .r = 223, .g = 113, .b =  38, .a = 255 },
            { .r = 217, .g = 160, .b = 102, .a = 255 },
            { .r = 238, .g = 195, .b = 154, .a = 255 },
            { .r = 251, .g = 242, .b =  54, .a = 255 },
            { .r = 153, .g = 229, .b =  80, .a = 255 },
            { .r = 106, .g = 190, .b =  48, .a = 255 },
            { .r =  55, .g = 148, .b = 110, .a = 255 },
            { .r =  75, .g = 105, .b =  47, .a = 255 },
            { .r =  82, .g =  75, .b =  36, .a = 255 },
            { .r =  50, .g =  60, .b =  57, .a = 255 },
            { .r =  63, .g =  63, .b = 116, .a = 255 },
            { .r =  48, .g =  96, .b = 130, .a = 255 },
            { .r =  91, .g = 110, .b = 225, .a = 255 },
            { .r =  99, .g = 155, .b = 255, .a = 255 },
            { .r =  95, .g = 205, .b = 228, .a = 255 },
            { .r = 203, .g = 219, .b = 252, .a = 255 },
            { .r = 255, .g = 255, .b = 255, .a = 255 },
            { .r = 155, .g = 173, .b = 183, .a = 255 },
            { .r = 132, .g = 126, .b = 135, .a = 255 },
            { .r = 105, .g = 106, .b = 106, .a = 255 },
            { .r =  89, .g =  86, .b =  82, .a = 255 },
            { .r = 118, .g =  66, .b = 138, .a = 255 },
            { .r = 172, .g =  50, .b =  50, .a = 255 },
            { .r = 217, .g =  87, .b =  99, .a = 255 },
            { .r = 215, .g = 123, .b = 186, .a = 255 },
            { .r = 143, .g = 151, .b =  74, .a = 255 },
            { .r = 138, .g = 111, .b =  48, .a = 255 }
        }, 32 }
    },
    { "6-bit-rgb", {
        {
            { .r =   0, .g =   0, .b =   0, .a = 255 },
            { .r =   0, .g =   0, .b =  85, .a = 255 },
            { .r =   0, .g =   0, .b = 170, .a = 255 },
            { .r =   0, .g =   0, .b = 255, .a = 255 },
            { .r =  85, .g =   0, .b =   0, .a = 255 },
            { .r =  85, .g =   0, .b =  85, .a = 255 },
            { .r =  85, .g =   0, .b = 170, .a = 255 },
            { .r =  85, .g =   0, .b = 255, .a = 255 },
            { .r = 170, .g =   0, .b =   0, .a = 255 },
            { .r = 170, .g =   0, .b =  85, .a = 255 },
            { .r = 170, .g =   0, .b = 170, .a = 255 },
            { .r = 170, .g =   0, .b = 255, .a = 255 },
            { .r = 255, .g =   0, .b =   0, .a = 255 },
            { .r = 255, .g =   0, .b =  85, .a = 255 },
            { .r = 255, .g =   0, .b = 170, .a = 255 },
            { .r = 255, .g =   0, .b = 255, .a = 255 },
            { .r =   0, .g =  85, .b =   0, .a = 255 },
            { .r =   0, .g =  85, .b =  85, .a = 255 },
            { .r =   0, .g =  85, .b = 170, .a = 255 },
            { .r =   0, .g =  85, .b = 255, .a = 255 },
            { .r =  85, .g =  85, .b =   0, .a = 255 },
            { .r =  85, .g =  85, .b =  85, .a = 255 },
            { .r =  85, .g =  85, .b = 170, .a = 255 },
            { .r =  85, .g =  85, .b = 255, .a = 255 },
            { .r = 170, .g =  85, .b =   0, .a = 255 },
            { .r = 170, .g =  85, .b =  85, .a = 255 },
            { .r = 170, .g =  85, .b = 170, .a = 255 },
            { .r = 170, .g =  85, .b = 255, .a = 255 },
            { .r = 255, .g =  85, .b =   0, .a = 255 },
            { .r = 255, .g =  85, .b =  85, .a = 255 },
            { .r = 255, .g =  85, .b = 170, .a = 255 },
            { .r = 255, .g =  85, .b = 255, .a = 255 },
            { .r =   0, .g = 170, .b =   0, .a = 255 },
            { .r =   0, .g = 170, .b =  85, .a = 255 },
            { .r =   0, .g = 170, .b = 170, .a = 255 },
            { .r =   0, .g = 170, .b = 255, .a = 255 },
            { .r =  85, .g = 170, .b =   0, .a = 255 },
            { .r =  85, .g = 170, .b =  85, .a = 255 },
            { .r =  85, .g = 170, .b = 170, .a = 255 },
            { .r =  85, .g = 170, .b = 255, .a = 255 },
            { .r = 170, .g = 170, .b =   0, .a = 255 },
            { .r = 170, .g = 170, .b =  85, .a = 255 },
            { .r = 170, .g = 170, .b = 170, .a = 255 },
            { .r = 170, .g = 170, .b = 255, .a = 255 },
            { .r = 255, .g = 170, .b =   0, .a = 255 },
            { .r = 255, .g = 170, .b =  85, .a = 255 },
            { .r = 255, .g = 170, .b = 170, .a = 255 },
            { .r = 255, .g = 170, .b = 255, .a = 255 },
            { .r =   0, .g = 255, .b =   0, .a = 255 },
            { .r =   0, .g = 255, .b =  85, .a = 255 },
            { .r =   0, .g = 255, .b = 170, .a = 255 },
            { .r =   0, .g = 255, .b = 255, .a = 255 },
            { .r =  85, .g = 255, .b =   0, .a = 255 },
            { .r =  85, .g = 255, .b =  85, .a = 255 },
            { .r =  85, .g = 255, .b = 170, .a = 255 },
            { .r =  85, .g = 255, .b = 255, .a = 255 },
            { .r = 170, .g = 255, .b =   0, .a = 255 },
            { .r = 170, .g = 255, .b =  85, .a = 255 },
            { .r = 170, .g = 255, .b = 170, .a = 255 },
            { .r = 170, .g = 255, .b = 255, .a = 255 },
            { .r = 255, .g = 255, .b =   0, .a = 255 },
            { .r = 255, .g = 255, .b =  85, .a = 255 },
            { .r = 255, .g = 255, .b = 170, .a = 255 },
            { .r = 255, .g = 255, .b = 255, .a = 255 },
        }, 64 }
    },
    { "nes", {
        {
            { .r = 124, .g = 124, .b = 124, .a = 255 },
            { .r =   0, .g =   0, .b = 252, .a = 255 },
            { .r =   0, .g =   0, .b = 188, .a = 255 },
            { .r =  68, .g =  40, .b = 188, .a = 255 },
            { .r = 148, .g =   0, .b = 132, .a = 255 },
            { .r = 168, .g =   0, .b =  32, .a = 255 },
            { .r = 168, .g =  16, .b =   0, .a = 255 },
            { .r = 136, .g =  20, .b =   0, .a = 255 },
            { .r =  80, .g =  48, .b =   0, .a = 255 },
            { .r =   0, .g = 120, .b =   0, .a = 255 },
            { .r =   0, .g = 104, .b =   0, .a = 255 },
            { .r =   0, .g =  88, .b =   0, .a = 255 },
            { .r =   0, .g =  64, .b =  88, .a = 255 },
            { .r =   0, .g =   0, .b =   0, .a = 255 },
            { .r =   0, .g =   0, .b =   0, .a = 255 },
            { .r =   0, .g =   0, .b =   0, .a = 255 },
            { .r = 188, .g = 188, .b = 188, .a = 255 },
            { .r =   0, .g = 120, .b = 248, .a = 255 },
            { .r =   0, .g =  88, .b = 248, .a = 255 },
            { .r = 104, .g =  68, .b = 252, .a = 255 },
            { .r = 216, .g =   0, .b = 204, .a = 255 },
            { .r = 228, .g =   0, .b =  88, .a = 255 },
            { .r = 248, .g =  56, .b =   0, .a = 255 },
            { .r = 228, .g =  92, .b =  16, .a = 255 },
            { .r = 172, .g = 124, .b =   0, .a = 255 },
            { .r =   0, .g = 184, .b =   0, .a = 255 },
            { .r =   0, .g = 168, .b =   0, .a = 255 },
            { .r =   0, .g = 168, .b =  68, .a = 255 },
            { .r =   0, .g = 136, .b = 136, .a = 255 },
            { .r =   0, .g =   0, .b =   0, .a = 255 },
            { .r =   0, .g =   0, .b =   0, .a = 255 },
            { .r =   0, .g =   0, .b =   0, .a = 255 },
            { .r = 248, .g = 248, .b = 248, .a = 255 },
            { .r =  60, .g = 188, .b = 252, .a = 255 },
            { .r = 104, .g = 136, .b = 252, .a = 255 },
            { .r = 152, .g = 120, .b = 248, .a = 255 },
            { .r = 248, .g = 120, .b = 248, .a = 255 },
            { .r = 248, .g =  88, .b = 152, .a = 255 },
            { .r = 248, .g = 120, .b =  88, .a = 255 },
            { .r = 252, .g = 160, .b =  68, .a = 255 },
            { .r = 248, .g = 184, .b =   0, .a = 255 },
            { .r = 184, .g = 248, .b =  24, .a = 255 },
            { .r =  88, .g = 216, .b =  84, .a = 255 },
            { .r =  88, .g = 248, .b = 152, .a = 255 },
            { .r =   0, .g = 232, .b = 216, .a = 255 },
            { .r = 120, .g = 120, .b = 120, .a = 255 },
            { .r =   0, .g =   0, .b =   0, .a = 255 },
            { .r =   0, .g =   0, .b =   0, .a = 255 },
            { .r = 252, .g = 252, .b = 252, .a = 255 },
            { .r = 164, .g = 228, .b = 252, .a = 255 },
            { .r = 184, .g = 184, .b = 248, .a = 255 },
            { .r = 216, .g = 184, .b = 248, .a = 255 },
            { .r = 248, .g = 184, .b = 248, .a = 255 },
            { .r = 248, .g = 164, .b = 192, .a = 255 },
            { .r = 240, .g = 208, .b = 176, .a = 255 },
            { .r = 252, .g = 224, .b = 168, .a = 255 },
            { .r = 248, .g = 216, .b = 120, .a = 255 },
            { .r = 216, .g = 248, .b = 120, .a = 255 },
            { .r = 184, .g = 248, .b = 184, .a = 255 },
            { .r = 184, .g = 248, .b = 216, .a = 255 },
            { .r =   0, .g = 252, .b = 252, .a = 255 },
            { .r = 248, .g = 216, .b = 248, .a = 255 },
            { .r =   0, .g =   0, .b =   0, .a = 255 },
            { .r =   0, .g =   0, .b =   0, .a = 255 }
        }, 64 },
    },
    { "famicube", {
        {
            { .r =   0, .g =   0, .b =   0, .a = 255 },
            { .r = 224, .g =  60, .b =  40, .a = 255 },
            { .r = 255, .g = 255, .b = 255, .a = 255 },
            { .r = 215, .g = 215, .b = 215, .a = 255 },
            { .r = 168, .g = 168, .b = 168, .a = 255 },
            { .r = 123, .g = 123, .b = 123, .a = 255 },
            { .r =  52, .g =  52, .b =  52, .a = 255 },
            { .r =  21, .g =  21, .b =  21, .a = 255 },
            { .r =  13, .g =  32, .b =  48, .a = 255 },
            { .r =  65, .g =  93, .b = 102, .a = 255 },
            { .r = 113, .g = 166, .b = 161, .a = 255 },
            { .r = 189, .g = 255, .b = 202, .a = 255 },
            { .r =  37, .g = 226, .b = 205, .a = 255 },
            { .r =  10, .g = 152, .b = 172, .a = 255 },
            { .r =   0, .g =  82, .b = 128, .a = 255 },
            { .r =   0, .g =  96, .b =  75, .a = 255 },
            { .r =  32, .g = 181, .b =  98, .a = 255 },
            { .r =  88, .g = 211, .b =  50, .a = 255 },
            { .r =  19, .g = 157, .b =   8, .a = 255 },
            { .r =   0, .g =  78, .b =   0, .a = 255 },
            { .r =  23, .g =  40, .b =   8, .a = 255 },
            { .r =  55, .g = 109, .b =   3, .a = 255 },
            { .r = 106, .g = 180, .b =  23, .a = 255 },
            { .r = 140, .g = 214, .b =  18, .a = 255 },
            { .r = 190, .g = 235, .b = 113, .a = 255 },
            { .r = 238, .g = 255, .b = 169, .a = 255 },
            { .r = 182, .g = 193, .b =  33, .a = 255 },
            { .r = 147, .g = 151, .b =  23, .a = 255 },
            { .r = 204, .g = 143, .b =  21, .a = 255 },
            { .r = 255, .g = 187, .b =  49, .a = 255 },
            { .r = 255, .g = 231, .b =  55, .a = 255 },
            { .r = 246, .g = 143, .b =  55, .a = 255 },
            { .r = 173, .g =  78, .b =  26, .a = 255 },
            { .r =  35, .g =  23, .b =  18, .a = 255 },
            { .r =  92, .g =  60, .b =  13, .a = 255 },
            { .r = 174, .g = 108, .b =  55, .a = 255 },
            { .r = 197, .g = 151, .b = 130, .a = 255 },
            { .r = 226, .g = 215, .b = 181, .a = 255 },
            { .r =  79, .g =  21, .b =   7, .a = 255 },
            { .r = 130, .g =  60, .b =  61, .a = 255 },
            { .r = 218, .g = 101, .b =  94, .a = 255 },
            { .r = 225, .g = 130, .b = 137, .a = 255 },
            { .r = 245, .g = 183, .b = 132, .a = 255 },
            { .r = 255, .g = 233, .b = 197, .a = 255 },
            { .r = 255, .g = 130, .b = 206, .a = 255 },
            { .r = 207, .g =  60, .b = 113, .a = 255 },
            { .r = 135, .g =  22, .b =  70, .a = 255 },
            { .r = 163, .g =  40, .b = 179, .a = 255 },
            { .r = 204, .g = 105, .b = 228, .a = 255 },
            { .r = 213, .g = 156, .b = 252, .a = 255 },
            { .r = 254, .g = 201, .b = 237, .a = 255 },
            { .r = 226, .g = 201, .b = 255, .a = 255 },
            { .r = 166, .g = 117, .b = 254, .a = 255 },
            { .r = 106, .g =  49, .b = 202, .a = 255 },
            { .r =  90, .g =  25, .b = 145, .a = 255 },
            { .r =  33, .g =  22, .b =  64, .a = 255 },
            { .r =  61, .g =  52, .b = 165, .a = 255 },
            { .r =  98, .g = 100, .b = 220, .a = 255 },
            { .r = 155, .g = 160, .b = 239, .a = 255 },
            { .r = 152, .g = 220, .b = 255, .a = 255 },
            { .r =  91, .g = 168, .b = 255, .a = 255 },
            { .r =  10, .g = 137, .b = 255, .a = 255 },
            { .r =   2, .g =  74, .b = 202, .a = 255 },
            { .r =   0, .g =  23, .b = 125, .a = 255 }
        }, 64 },
    },
    { NULL, { { { 0 } }, 0 } }
};

const GL_Palette_t *resources_palettes_find(const char *id)
{
    for (size_t i = 0; _palettes[i].id != NULL; ++i) {
        if (strcasecmp(_palettes[i].id, id) == 0) {
            return &_palettes[i].palette;
        }
    }
    return NULL;
}
