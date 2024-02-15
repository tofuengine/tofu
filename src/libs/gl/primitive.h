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
 * Copyright (c) 2019-2024 Marco Lizza
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

#ifndef TOFU_LIBS_GL_CONTEXT_PRIMITIVE_H
#define TOFU_LIBS_GL_CONTEXT_PRIMITIVE_H

#include "common.h"
#include "context.h"

extern void GL_context_point(const GL_Context_t *context, GL_Point_t position, GL_Pixel_t index);
extern void GL_context_hline(const GL_Context_t *context, GL_Point_t origin,  size_t w, GL_Pixel_t index);
extern void GL_context_vline(const GL_Context_t *context, GL_Point_t origin, size_t h, GL_Pixel_t index);
extern void GL_context_polyline(const GL_Context_t *context, const GL_Point_t *vertices, size_t count, GL_Pixel_t index);

extern void GL_context_filled_rectangle(const GL_Context_t *context, GL_Rectangle_t rectangle, GL_Pixel_t index);
extern void GL_context_filled_triangle(const GL_Context_t *context, GL_Point_t v0, GL_Point_t v1, GL_Point_t v2, GL_Pixel_t index);
//extern void GL_context_filled_fan(const GL_Context_t *context, GL_Point_t a, GL_Point_t b, GL_Point_t c, GL_Pixel_t index);
extern void GL_context_filled_circle(const GL_Context_t *context, GL_Point_t center, size_t radius, GL_Pixel_t index);
extern void GL_context_circle(const GL_Context_t *context, GL_Point_t center, size_t radius, GL_Pixel_t index);

#endif  /* TOFU_LIBS_GL_CONTEXT_PRIMITIVE_H */
