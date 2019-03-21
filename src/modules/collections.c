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

#include "collections.h"

#include "collections.h"

#include "../memory.h"

const char collections_wren[] =
    "foreign class Grid {\n"
    "\n"
    "    construct new(width, height) {}\n"
    "\n"
    "    foreign width\n"
    "    foreign height\n"
    "    foreign fill(value)\n"
    "    foreign peek(x, y)\n"
    "    foreign poke(x, y, value)\n"
    "\n"
    "}\n"
;

typedef struct _Grid_t {
    int width;
    int height;
    int *data;
    int **offsets; // Precomputed pointers to the line of data.
} Grid_t;

void grid_allocate(WrenVM* vm)
{
    int width = (int)wrenGetSlotDouble(vm, 1);
    int height = (int)wrenGetSlotDouble(vm, 2);
    int *data = Memory_alloc(sizeof(int) * width * height);
    int **offsets = Memory_alloc(sizeof(int *) * height);

    for (int i = 0; i < height; ++i) { // Precompute the pointers to the data rows for faster access (old-school! :D).
        offsets[i] = data + (i * width);
    }

    Grid_t* grid = (Grid_t *)wrenSetSlotNewForeign(vm, 0, 0, sizeof(Grid_t)); // `0, 0` since we are in the allocate callback.
    *grid = (Grid_t){ width, height, data, offsets };
}

void grid_finalize(void* data)
{
    Grid_t* grid = (Grid_t *)data;
    Memory_free(grid->data);
    Memory_free(grid->offsets);
}

void grid_width(WrenVM *vm)
{
    Grid_t* grid = (Grid_t *)wrenGetSlotForeign(vm, 0);

    wrenSetSlotDouble(vm, 0, grid->width);
}

void grid_height(WrenVM *vm)
{
    Grid_t* grid = (Grid_t *)wrenGetSlotForeign(vm, 0);

    wrenSetSlotDouble(vm, 0, grid->height);
}

void grid_fill(WrenVM *vm)
{
    Grid_t* grid = (Grid_t *)wrenGetSlotForeign(vm, 0);

    int value = (int)wrenGetSlotDouble(vm, 1);

    int width = grid->width;
    int height = grid->height;
    int *data = grid->data;

    int size = width * height;
    for (int i = 0; i < size; ++i) {
        data[i] = value;
    }
}

void grid_peek(WrenVM *vm)
{
    Grid_t* grid = (Grid_t *)wrenGetSlotForeign(vm, 0);

    int x = (int)wrenGetSlotDouble(vm, 1);
    int y = (int)wrenGetSlotDouble(vm, 2);

    int *ptr = grid->offsets[y];

    int value = ptr[x];

    wrenSetSlotDouble(vm, 0, value);
}

void grid_poke(WrenVM *vm)
{
    Grid_t* grid = (Grid_t *)wrenGetSlotForeign(vm, 0);

    int x = (int)wrenGetSlotDouble(vm, 1);
    int y = (int)wrenGetSlotDouble(vm, 2);
    int value = (int)wrenGetSlotDouble(vm, 3);

    int *ptr = grid->offsets[y];

    ptr[x] = value;
}
