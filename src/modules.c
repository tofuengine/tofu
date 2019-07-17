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

#include "modules.h"

#include "modules/collections.h"
#include "modules/events.h"
#include "modules/graphics.h"
#include "modules/io.h"
#include "modules/util.h"

const Module_Entry_t _modules[] = {
//  { "<module-name>", "<module-source>" }
    { "collections", collections_wren },
    { "events", events_wren },
    { "graphics", graphics_wren },
    { "io", io_wren },
    { "util", util_wren },
    { NULL, NULL }
};
const Class_Entry_t _classes[] = {
//  { "<module-name>", "<class-name>", <allocator>, <deallocator> }
    { "collections", "Grid", collections_grid_allocate, collections_grid_finalize },
    { "graphics", "Bank", graphics_bank_allocate, graphics_bank_finalize },
    { "graphics", "Font", graphics_font_allocate, graphics_font_finalize },
    { "util", "Timer", util_timer_allocate, util_timer_finalize },
    { NULL, NULL, NULL, NULL }
};
const Method_Entry_t _methods[] = {
//  { "<module-name>", "<class-name>", <is-static>, "<signature>", <function> }
    { "collections", "Grid", false, "width", collections_grid_width_get },
    { "collections", "Grid", false, "height", collections_grid_height_get },
    { "collections", "Grid", false, "fill(_)", collections_grid_fill_call1 },
    { "collections", "Grid", false, "stride(_,_,_,_)", collections_grid_stride_call4 },
    { "collections", "Grid", false, "peek(_,_)", collections_grid_peek_call2 },
    { "collections", "Grid", false, "poke(_,_,_)", collections_grid_poke_call3 },
    { "events", "Input", true, "isKeyDown(_)", events_input_iskeydown_call1 },
    { "events", "Input", true, "isKeyUp(_)", events_input_iskeyup_call1 },
    { "events", "Input", true, "isKeyPressed(_)", events_input_iskeypressed_call1 },
    { "events", "Input", true, "isKeyReleased(_)", events_input_iskeyreleased_call1 },
    { "events", "Environment", true, "fps", events_environment_fps_get },
    { "events", "Environment", true, "quit()", events_environment_quit_call0 },
    { "graphics", "Bank", false, "cellWidth", graphics_bank_cell_width_get },
    { "graphics", "Bank", false, "cellHeight", graphics_bank_cell_height_get },
    { "graphics", "Bank", false, "blit(_,_,_)", graphics_bank_blit_call3 },
    { "graphics", "Bank", false, "blit(_,_,_,_,_)", graphics_bank_blit_call5 },
    { "graphics", "Bank", false, "blit(_,_,_,_,_,_)", graphics_bank_blit_call6 },
    { "graphics", "Font", false, "write(_,_,_,_,_,_)", graphics_font_write_call6 },
    { "graphics", "Canvas", true, "width", graphics_canvas_width_get },
    { "graphics", "Canvas", true, "height", graphics_canvas_height_get },
    { "graphics", "Canvas", true, "palette", graphics_canvas_palette_get },
    { "graphics", "Canvas", true, "palette=(_)", graphics_canvas_palette_set },
    { "graphics", "Canvas", true, "background=(_)", graphics_canvas_background_set },
    { "graphics", "Canvas", true, "shader=(_)", graphics_canvas_shader_set },
    { "graphics", "Canvas", true, "color(_)", graphics_canvas_color_call1 },
    { "graphics", "Canvas", true, "points(_,_)", graphics_canvas_points_call2 },
    { "graphics", "Canvas", true, "polyline(_,_)", graphics_canvas_polyline_call2 },
    { "graphics", "Canvas", true, "strip(_,_)", graphics_canvas_strip_call2 },
    { "graphics", "Canvas", true, "fan(_,_)", graphics_canvas_fan_call2 },
    { "io", "File", true, "read(_)", io_file_read_call1 },
    { "util", "Timer", false, "reset()", util_timer_reset_call0 },
    { "util", "Timer", false, "cancel()", util_timer_cancel_call0 },
    { NULL, NULL, false, NULL, NULL }
};
