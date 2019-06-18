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
extern void graphics_bank_finalize(void *userData, void* data);
extern void graphics_bank_cell_width_get(WrenVM *vm);
extern void graphics_bank_cell_height_get(WrenVM *vm);
extern void graphics_bank_hot_spot_get(WrenVM *vm);
extern void graphics_bank_hot_spot_set(WrenVM *vm);
extern void graphics_bank_blit_call3(WrenVM *vm);
extern void graphics_bank_blit_call5(WrenVM *vm);
extern void graphics_bank_blit_call6(WrenVM *vm);

extern void graphics_font_allocate(WrenVM* vm);
extern void graphics_font_finalize(void *userData, void* data);
extern void graphics_font_write_call6(WrenVM *vm);

extern void graphics_canvas_width_get(WrenVM *vm);
extern void graphics_canvas_height_get(WrenVM *vm);
extern void graphics_canvas_palette_get(WrenVM *vm);
extern void graphics_canvas_palette_set(WrenVM *vm);
extern void graphics_canvas_background_set(WrenVM *vm);
extern void graphics_canvas_shader_set(WrenVM *vm);
extern void graphics_canvas_points_call2(WrenVM *vm); // TODO: should points, polygons and circles be objects?
extern void graphics_canvas_polyline_call2(WrenVM *vm);
extern void graphics_canvas_strip_call2(WrenVM *vm);
extern void graphics_canvas_fan_call2(WrenVM *vm);

#endif  /* __MODULES_GRAPHICS_H__ */