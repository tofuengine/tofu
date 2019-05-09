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
    "    construct new(width, height, content) {}\n"
    "\n"
    "    foreign width\n"
    "    foreign height\n"
    "    foreign fill(content)\n"
    "    foreign stride(column, row, content, amount)\n"
    "    foreign peek(column, row)\n"
    "    foreign poke(column, row, value)\n"
    "\n"
    "}\n"
;

typedef int Cell_t; // TODO: Is `double` be better?

typedef struct _Grid_Class_t {
    int width;
    int height;
    Cell_t *data;
    Cell_t **offsets; // Precomputed pointers to the line of data.
} Grid_Class_t;

void collections_grid_allocate(WrenVM *vm)
{
    int width = (int)wrenGetSlotDouble(vm, 1);
    int height = (int)wrenGetSlotDouble(vm, 2);
    WrenType type = wrenGetSlotType(vm, 3);

    Grid_Class_t *instance = (Grid_Class_t *)wrenSetSlotNewForeign(vm, 0, 0, sizeof(Grid_Class_t)); // `0, 0` since we are in the allocate callback.

    Cell_t *data = Memory_alloc(sizeof(Cell_t) * width * height);
    Cell_t **offsets = Memory_alloc(sizeof(Cell_t *) * height);

    for (int i = 0; i < height; ++i) { // Precompute the pointers to the data rows for faster access (old-school! :D).
        offsets[i] = data + (i * width);
    }

    Cell_t *ptr = data;
    Cell_t *eod = ptr + (width * height);

    if (type == WREN_TYPE_LIST) {
        int count = wrenGetListCount(vm, 3);
#ifdef __DEBUG_VM_CALLS__
        Log_write(LOG_LEVELS_DEBUG, "List has #%d elements(s)", count);
#endif

        int slots = wrenGetSlotCount(vm);
#ifdef __DEBUG_VM_CALLS__
        Log_write(LOG_LEVELS_DEBUG, "Currently #%d slot(s) available, asking for an additional slot", slots);
#endif
        const int aux_slot_id = slots;
        wrenEnsureSlots(vm, aux_slot_id + 1); // Ask for an additional temporary slot.

        // FIXME: At the moment Wren has a bug that prevents the foreign constructor to "modify" the VM (for
        //        example to request an additional slot). This code doesn't work, but we leave it here in the
        //        case the interpreter will be fixed.

#ifdef __GRID_REPEAT_CONTENT__
        for (int i = 0; (ptr < eod) && (count > 0); ++i) {
            wrenGetListElement(vm, 3, i % count, aux_slot_id);

            Cell_t value = (Cell_t)wrenGetSlotDouble(vm, aux_slot_id);

            *(ptr++) = value;
        }
#else
        for (int i = 0; (ptr < eod) && (i < count); ++i) { // Don't exceed buffer if list is too long to fit!
            wrenGetListElement(vm, 3, i, aux_slot_id);

            Cell_t value = (Cell_t)wrenGetSlotDouble(vm, aux_slot_id);

            *(ptr++) = value;
        }
#endif
    } else
    if (type == WREN_TYPE_NUM) {
        Cell_t value = (Cell_t)wrenGetSlotDouble(vm, 3);

        while (ptr < eod) {
            *(ptr++) = value;
        }
    }

    *instance = (Grid_Class_t){
            .width = width,
            .height = height,
            .data = data,
            .offsets = offsets
        };
}

void collections_grid_finalize(void *userData, void *data)
{
    Grid_Class_t *instance = (Grid_Class_t *)data;
    Memory_free(instance->data);
    Memory_free(instance->offsets);
}

void collections_grid_width(WrenVM *vm)
{
    Grid_Class_t *instance = (Grid_Class_t *)wrenGetSlotForeign(vm, 0);

    wrenSetSlotDouble(vm, 0, instance->width);
}

void collections_grid_height(WrenVM *vm)
{
    Grid_Class_t *instance = (Grid_Class_t *)wrenGetSlotForeign(vm, 0);

    wrenSetSlotDouble(vm, 0, instance->height);
}

void collections_grid_fill(WrenVM *vm)
{
    WrenType type = wrenGetSlotType(vm, 1);

    Grid_Class_t *instance = (Grid_Class_t *)wrenGetSlotForeign(vm, 0);

    int width = instance->width;
    int height = instance->height;
    Cell_t *data = instance->data;

    Cell_t *ptr = data;
    Cell_t *eod = ptr + (width * height);

    if (type == WREN_TYPE_LIST) {
        int count = wrenGetListCount(vm, 1);

        int slots = wrenGetSlotCount(vm);
        const int aux_slot_id = slots;
#ifdef __DEBUG_VM_CALLS__
        Log_write(LOG_LEVELS_DEBUG, "Currently #%d slot(s) available, asking for additional slot", slots);
#endif
        wrenEnsureSlots(vm, aux_slot_id + 1); // Ask for an additional temporary slot.

#ifdef __GRID_REPEAT_CONTENT__
        for (int i = 0; (ptr < eod) && (count > 0); ++i) {
            wrenGetListElement(vm, 1, i % count, aux_slot_id);

            Cell_t value = (Cell_t)wrenGetSlotDouble(vm, aux_slot_id);

            *(ptr++) = value;
        }
#else
        for (int i = 0; (ptr < eod) && (i < count); ++i) { // Don't exceed buffer if list is too long to fit!
            wrenGetListElement(vm, 1, i, aux_slot_id);

            Cell_t value = (Cell_t)wrenGetSlotDouble(vm, aux_slot_id);

            *(ptr++) = value;
        }
#endif
    } else
    if (type == WREN_TYPE_NUM) {
        Cell_t value = (Cell_t)wrenGetSlotDouble(vm, 1);

        while (ptr < eod) {
            *(ptr++) = value;
        }
    }
}

void collections_grid_stride(WrenVM *vm)
{
    int column = (int)wrenGetSlotDouble(vm, 1);
    int row = (int)wrenGetSlotDouble(vm, 2);
    WrenType type = wrenGetSlotType(vm, 3);
    int amount = (int)wrenGetSlotDouble(vm, 4);

    Grid_Class_t *instance = (Grid_Class_t *)wrenGetSlotForeign(vm, 0);

    int width = instance->width;
    int height = instance->height;
    Cell_t *data = instance->offsets[row];

    Cell_t *ptr = data + column;
    Cell_t *eod = ptr + (width * height);

    if (type == WREN_TYPE_LIST) {
        int count = wrenGetListCount(vm, 3);

        int slots = wrenGetSlotCount(vm);
        const int aux_slot_id = slots;
#ifdef __DEBUG_VM_CALLS__
        Log_write(LOG_LEVELS_DEBUG, "Currently #%d slot(s) available, asking for additional slot", slots);
#endif
        wrenEnsureSlots(vm, aux_slot_id + 1); // Ask for an additional temporary slot.

#ifdef __GRID_REPEAT_CONTENT__
        for (int i = 0; (ptr < eod) && (i < amount) && (count > 0); ++i) {
            wrenGetListElement(vm, 1, i % count, aux_slot_id);

            Cell_t value = (Cell_t)wrenGetSlotDouble(vm, aux_slot_id);

            *(ptr++) = value;
        }
#else
        for (int i = 0; (ptr < eod) && (i < amount) && (i < count); ++i) { // Don't exceed buffer if list is too long to fit!
            wrenGetListElement(vm, 3, i, aux_slot_id);

            Cell_t value = (Cell_t)wrenGetSlotDouble(vm, aux_slot_id);

            *(ptr++) = value;
        }
#endif
    } else
    if (type == WREN_TYPE_NUM) {
        Cell_t value = (Cell_t)wrenGetSlotDouble(vm, 3);

        for (int i = 0; (ptr < eod) && (i < amount); ++i) {
            *(ptr++) = value;
        }
    }
}

void collections_grid_peek(WrenVM *vm)
{
    int column = (int)wrenGetSlotDouble(vm, 1);
    int row = (int)wrenGetSlotDouble(vm, 2);

    Grid_Class_t *instance = (Grid_Class_t *)wrenGetSlotForeign(vm, 0);

    Cell_t *data = instance->offsets[row];

    Cell_t value = data[column];

    wrenSetSlotDouble(vm, 0, value);
}

void collections_grid_poke(WrenVM *vm)
{
    int column = (int)wrenGetSlotDouble(vm, 1);
    int row = (int)wrenGetSlotDouble(vm, 2);
    Cell_t value = (Cell_t)wrenGetSlotDouble(vm, 3);

    Grid_Class_t *instance = (Grid_Class_t *)wrenGetSlotForeign(vm, 0);

    Cell_t *data = instance->offsets[row];

    data[column] = value;
}
