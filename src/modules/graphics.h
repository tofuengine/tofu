/*
 * Copyright (c) 2019 Marco Lizza (marco.lizza@gmail.com)
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
 **/

#ifndef __MODULES_GRAPHICS_H__
#define __MODULES_GRAPHICS_H__

#include <wren/wren.h>

extern const char graphics_wren[];

extern void graphics_bank_allocate(WrenVM* vm);
extern void graphics_bank_finalize(void* data);
extern void graphics_bank_draw(WrenVM *vm); // TODO: is `Bank.blit()` any better?
extern void graphics_bank_cell_width(WrenVM *vm);
extern void graphics_bank_cell_height(WrenVM *vm);

extern void graphics_font_allocate(WrenVM* vm);
extern void graphics_font_finalize(void* data);
extern void graphics_font_text(WrenVM *vm); // TODO: rename to `Font.write()`?

extern void graphics_canvas_width(WrenVM *vm);
extern void graphics_canvas_height(WrenVM *vm);
extern void graphics_canvas_palette(WrenVM *vm);
extern void graphics_canvas_point(WrenVM *vm);
extern void graphics_canvas_polygon(WrenVM *vm);
extern void graphics_canvas_circle(WrenVM *vm);

#endif  /* __MODULES_GRAPHICS_H__ */