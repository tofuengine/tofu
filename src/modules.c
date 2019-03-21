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
#include "modules/graphics.h"
#include "modules/events.h"

const Module_Entry_t _modules[] = {
//  { "module", NULL }
    { "collections", collections_wren },
    { "graphics", graphics_wren },
    { "events", events_wren },
    { NULL, NULL }
};
const Class_Entry_t _classes[] = {
//  { "module", "className", NULL, NULL }
    { "collections", "Grid", grid_allocate, grid_finalize },
    { NULL, NULL, NULL, NULL }
};
const Method_Entry_t _methods[] = {
//  { "module", "className", true, "update()", NULL }
    { "collections", "Grid", false, "fill(_)", grid_fill },
    { "collections", "Grid", false, "peek(_,_)", grid_peek },
    { "collections", "Grid", false, "poke(_,_,_)", grid_poke },
    { "graphics", "Canvas", true, "width", graphics_canvas_width },
    { "graphics", "Canvas", true, "height", graphics_canvas_height },
    { "graphics", "Canvas", true, "palette(_)", graphics_canvas_palette },
    { "graphics", "Canvas", true, "font(_,_)", graphics_canvas_font },
    { "graphics", "Canvas", true, "bank(_,_,_,_)", graphics_canvas_bank },
    { "graphics", "Canvas", true, "defaultFont", graphics_canvas_defaultFont },
    { "graphics", "Canvas", true, "text(_,_,_,_,_,_,_)", graphics_canvas_text },
    { "graphics", "Canvas", true, "point(_,_,_)", graphics_canvas_point },
    { "graphics", "Canvas", true, "polygon(_,_,_)", graphics_canvas_polygon },
    { "graphics", "Canvas", true, "circle(_,_,_,_,_)", graphics_canvas_circle },
    { "graphics", "Canvas", true, "sprite(_,_,_,_,_,_,_)", graphics_canvas_sprite },
    { "events", "Input", true, "isKeyDown(_)", events_input_iskeydown },
    { "events", "Input", true, "isKeyUp(_)", events_input_iskeyup },
    { "events", "Input", true, "isKeyPressed(_)", events_input_iskeypressed },
    { "events", "Input", true, "isKeyReleased(_)", events_input_iskeyreleased },
    { "events", "Environment", true, "quit()", events_environment_quit },
    { NULL, NULL, false, NULL, NULL }
};
