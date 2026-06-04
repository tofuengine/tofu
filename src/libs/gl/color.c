/*
 *                 ___________________  _______________ ___
 *                 \__    ___/\_____  \ \_   _____/    |   \
 *                   |    |    /   |   \ |    __) |    |   /
 *                   |    |   /    |    \|     \  |    |  /
 *                   |____|   \_______  /\___  /  |______/
 *                                    \/     \/
 *         ___________ _______    ________.___ _______  ___________
 *         \_   _____/ \      \  /  _____/|   |\      \ \_   _____/
 *          |    __)_  /   |   \/   \  ___|   |/   |   \ |    __)_
 *          |        \/    |    \    \_\  \   /    |    \|        \
 *         /_______  /\____|__  /\______  /___\____|__  /_______  /
 *                 \/         \/        \/            \/        \
 *
 * MIT License
 * 
 * Copyright (c) 2019-2026 Marco Lizza
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

#include "color.h"

#include <core/config.h>

GL_Color_t gl_color_from_rgb(uint8_t r, uint8_t g, uint8_t b)
{
#if TOFU_GRAPHICS_PIXEL_FORMAT == TOFU_PIXEL_FORMAT_RGBA8888
    return (GL_Color_t){ .r = r, .g = g, .b = b, .a = 255 };
#elif TOFU_GRAPHICS_PIXEL_FORMAT == TOFU_PIXEL_FORMAT_RGB565
    //      1
    // 5432109876543210
    // RRRRRGGGGGGBBBBB
    return ((r << 8) & 0xF800)
            | ((g << 3) & 0x07E0)
            | ((b >> 3) & 0x001F);
#else
    #error "unsupported TOFU_GRAPHICS_PIXEL_FORMAT"
#endif
}

uint8_t gl_color_get_r(GL_Color_t color)
{
#if TOFU_GRAPHICS_PIXEL_FORMAT == TOFU_PIXEL_FORMAT_RGBA8888
    return color.r;
#elif TOFU_GRAPHICS_PIXEL_FORMAT == TOFU_PIXEL_FORMAT_RGB565
    return (color & 0xF800) >> 8
            | (color & 0xE000) >> 13;
#else
    #error "unsupported TOFU_GRAPHICS_PIXEL_FORMAT"
#endif
}

uint8_t gl_color_get_g(GL_Color_t color)
{
#if TOFU_GRAPHICS_PIXEL_FORMAT == TOFU_PIXEL_FORMAT_RGBA8888
    return color.g;
#elif TOFU_GRAPHICS_PIXEL_FORMAT == TOFU_PIXEL_FORMAT_RGB565
    return (color & 0x07E0) >> 3
            | (color & 0x0600) >> 9;
#else
    #error "unsupported TOFU_GRAPHICS_PIXEL_FORMAT"
#endif
}

uint8_t gl_color_get_b(GL_Color_t color)
{
#if TOFU_GRAPHICS_PIXEL_FORMAT == TOFU_PIXEL_FORMAT_RGBA8888
    return color.b;
#elif TOFU_GRAPHICS_PIXEL_FORMAT == TOFU_PIXEL_FORMAT_RGB565
    return (color & 0x001F) << 3
            | (color & 0x001C) >> 2;
#else
    #error "unsupported TOFU_GRAPHICS_PIXEL_FORMAT"
#endif
}
