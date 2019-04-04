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

#include "../log.h"
#include "../memory.h"

const char collections_wren[] =
    "foreign class Grid {\n"
    "\n"
    "    construct new(width, height) {}\n"
    "\n"
    "    foreign width\n"
    "    foreign height\n"
    "    foreign fill(valueOrList, offset, length)\n"
    "    foreign row(x, y, count, value)\n"
    "    foreign peek(x, y)\n"
    "    foreign poke(x, y, value)\n"
    "\n"
    "    fill(valueOrList) {\n"
    "        fill(valueOrList, 0, -1)\n"
    "    }\n"
    "\n"
    "}\n"
;

typedef int Cell_t; // TODO: Could double?

typedef struct _Grid_t {
    int width;
    int height;
    Cell_t *data;
    Cell_t **offsets; // Precomputed pointers to the line of data.
} Grid_t;

void collections_grid_allocate(WrenVM *vm)
{
    int width = (int)wrenGetSlotDouble(vm, 1);
    int height = (int)wrenGetSlotDouble(vm, 2);
    // TODO: pass the (optional) grid content as a list.
    Cell_t *data = Memory_alloc(sizeof(Cell_t) * width * height);
    Cell_t **offsets = Memory_alloc(sizeof(Cell_t *) * height);

    for (int i = 0; i < height; ++i) { // Precompute the pointers to the data rows for faster access (old-school! :D).
        offsets[i] = data + (i * width);
    }

    Grid_t *grid = (Grid_t *)wrenSetSlotNewForeign(vm, 0, 0, sizeof(Grid_t)); // `0, 0` since we are in the allocate callback.
    *grid = (Grid_t){
            .width = width,
            .height = height,
            .data = data,
            .offsets = offsets
        };
}

void collections_grid_finalize(void *data)
{
    Grid_t *grid = (Grid_t *)data;
    Memory_free(grid->data);
    Memory_free(grid->offsets);
}

void collections_grid_width(WrenVM *vm)
{
    Grid_t *grid = (Grid_t *)wrenGetSlotForeign(vm, 0);

    wrenSetSlotDouble(vm, 0, grid->width);
}

void collections_grid_height(WrenVM *vm)
{
    Grid_t *grid = (Grid_t *)wrenGetSlotForeign(vm, 0);

    wrenSetSlotDouble(vm, 0, grid->height);
}

void collections_grid_fill(WrenVM *vm)
{
    Grid_t *grid = (Grid_t *)wrenGetSlotForeign(vm, 0);

    WrenType type = wrenGetSlotType(vm, 1);
    int offset = (int)wrenGetSlotDouble(vm, 2);
    int length = (int)wrenGetSlotDouble(vm, 3);

    int width = grid->width;
    int height = grid->height;
    Cell_t *data = grid->data;

    Cell_t *ptr = data + offset;
    Cell_t *eod = ptr + (width * height);

    if (type == WREN_TYPE_LIST) {
        int count = wrenGetListCount(vm, 1);

        int slots = wrenGetSlotCount(vm);
        const int aux_slot_id = slots;
#ifdef DEBUG
        Log_write(LOG_LEVELS_DEBUG, "Currently #%d slot(s) available, asking for additional slot", slots);
#endif
        wrenEnsureSlots(vm, aux_slot_id + 1); // Ask for an additional temporary slot.

        for (int i = 0; (ptr < eod) && ((length < 0) || (i < length)); ++i) { // Copy the list, repeating if necessary.
            wrenGetListElement(vm, 1, i % count, aux_slot_id);

            Cell_t value = (Cell_t)wrenGetSlotDouble(vm, aux_slot_id);

            *(ptr++) = value;
        }
    } else {
        Cell_t value = (Cell_t)wrenGetSlotDouble(vm, 1);

        for (int i = 0; (ptr < eod) && ((length < 0) || (i < length)); ++i) {
            *(ptr++) = value;
        }
    }
}

void collections_grid_row(WrenVM *vm)
{
    Grid_t *grid = (Grid_t *)wrenGetSlotForeign(vm, 0);

    int x = (int)wrenGetSlotDouble(vm, 1);
    int y = (int)wrenGetSlotDouble(vm, 2);
    int count = (int)wrenGetSlotDouble(vm, 3);
    int value = (Cell_t)wrenGetSlotDouble(vm, 4);

    Cell_t *data = grid->offsets[y];

    Cell_t *ptr = data + x;
    Cell_t *eod = ptr + count;
    while (ptr < eod) {
        *(ptr++) = value;
    }
}

void collections_grid_peek(WrenVM *vm)
{
    Grid_t *grid = (Grid_t *)wrenGetSlotForeign(vm, 0);

    int x = (int)wrenGetSlotDouble(vm, 1);
    int y = (int)wrenGetSlotDouble(vm, 2);

    Cell_t *data = grid->offsets[y];

    Cell_t value = data[x];

    wrenSetSlotDouble(vm, 0, value);
}

void collections_grid_poke(WrenVM *vm)
{
    Grid_t *grid = (Grid_t *)wrenGetSlotForeign(vm, 0);

    int x = (int)wrenGetSlotDouble(vm, 1);
    int y = (int)wrenGetSlotDouble(vm, 2);
    Cell_t value = (Cell_t)wrenGetSlotDouble(vm, 3);

    Cell_t *data = grid->offsets[y];

    data[x] = value;
}
