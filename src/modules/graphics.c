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

#include "graphics.h"

#include "../config.h"
#include "../environment.h"
#include "../log.h"
#include "../memory.h"
#include "../gl/gl.h"
#include "graphics/palettes.h"

#include <math.h>
#include <string.h>

#define RED_WEIGHT      2.0f
#define GREEN_WEIGHT    4.0f
#define BLUE_WEIGHT     3.0f

// https://en.wikipedia.org/wiki/Color_difference
static size_t find_nearest_color(const GL_Palette_t *palette, GL_Color_t color)
{
    size_t index = 0;
    double minimum = __DBL_MAX__;
    for (size_t i = 0; i < palette->count; ++i) {
        const GL_Color_t *current = &palette->colors[i];

        double delta_r = (double)(color.r - current->r);
        double delta_g = (double)(color.g - current->g);
        double delta_b = (double)(color.b - current->b);
#ifdef __FIND_NEAREST_COLOR_EUCLIDIAN__
        double distance = sqrt((delta_r * delta_r) * RED_WEIGHT
            + (delta_g * delta_g) * GREEN_WEIGHT
            + (delta_b * delta_b)) * BLUE_WEIGHT;
#else
        double distance = (delta_r * delta_r) * RED_WEIGHT
            + (delta_g * delta_g) * GREEN_WEIGHT
            + (delta_b * delta_b) * BLUE_WEIGHT; // Faster, no need to get the Euclidean distance.
#endif
        if (minimum > distance) {
            minimum = distance;
            index = i;
        }
    }
    return index;
}

// TODO: convert image with a shader.
static void to_indexed_atlas_callback(void *parameters, void *data, int width, int height)
{
    const GL_Palette_t *palette = (const GL_Palette_t *)parameters;

    GL_Color_t *pixels = (GL_Color_t *)data;

    for (int y = 0; y < height; ++y) {
        int row_offset = width * y;
        for (int x = 0; x < width; ++x) {
            int offset = row_offset + x;

            GL_Color_t color = pixels[offset];
            if (color.a == 0) { // Skip transparent colors.
                continue;
            }

            size_t index = find_nearest_color(palette, color);
            pixels[offset] = (GL_Color_t){ index, index, index, color.a };
        }
    }
}

const char graphics_wren[] =
    "foreign class Bank {\n"
    "\n"
    "    construct new(file, cellWidth, cellHeight) {}\n"
    "\n"
    "    foreign cellWidth\n"
    "    foreign cellHeight\n"
    "\n"
    "    blit(cellId, x, y, r) {\n"
    "        blit(cellId, x, y, r, 1.0, 1.0)\n"
    "    }\n"
    "    foreign blit(cellId, x, y)\n"
    "    foreign blit(cellId, x, y, sx, sy)\n"
    "    foreign blit(cellId, x, y, r, sx, sy)\n"
    "\n"
    "}\n"
    "\n"
    "foreign class Font {\n"
    "\n"
    "    construct new(file, glyphWidth, glyphHeight) {}\n"
    "\n"
    "    static default { Font.new(\"default\", 0, 0) }\n"
    "\n"
    "    foreign write(text, x, y, color, scale, align)\n"
    "\n"
    "}\n"
    "\n"
    "foreign class Canvas {\n"
    "\n"
    "    foreign static width\n"
    "    foreign static height\n"
    "    foreign static palette\n"
    "    foreign static palette=(colors)\n"
    "\n"
    "    foreign static points(vertices, color)\n"
    "    foreign static polyline(vertices, color)\n"
    "    foreign static strip(vertices, color)\n"
    "    foreign static fan(vertices, color)\n"
    "\n"
    "    static point(x0, y0, color) {\n"
    "        points([ x0, y0 ], color)\n"
    "    }\n"
    "    static line(x0, y0, x1, y1, color) {\n"
    "        polyline([ x0, y0, x1, y1, x0, y0 ], color)\n"
    "    }\n"
    "    static triangle(mode, x0, y0, x1, y1, x2, y2, color) {\n"
    "        if (mode == \"line\") {\n"
    "            polyline([ x0, y0, x1, y1, x2, y2, x0, y0 ], color)\n"
    "        } else {\n"
    "            strip([ x0, y0, x1, y1, x2, y2 ], color)\n"
    "        }\n"
    "    }\n"
    "    static rectangle(mode, x, y, width, height, color) {\n"
    "        var offset = mode == \"line\" ? 1 : 0\n"
    "        var x0 = x\n"
    "        var y0 = y\n"
    "        var x1 = x0 + width - offset\n"
    "        var y1= y0 + height - offset\n"
    "        if (mode == \"line\") {\n"
    "            polyline([ x0, y0, x0, y1, x1, y1, x1, y0, x0, y0 ], color)\n"
    "        } else {\n"
    "            strip([ x0, y0, x0, y1, x1, y0, x1, y1 ], color)\n"
    "        }\n"
    "    }\n"
    "    static square(mode, x, y, size, color) {\n"
    "        rectangle(mode, x, y, size, size, color)\n"
    "    }\n"
    "    static circle(mode, x, y, radius, color) {\n"
    "        circle(mode, x, y, radius, color, 30)\n"
    "    }\n"
    "    static circle(mode, x, y, radius, color, segments) {\n"
    "        var step = (2 * Num.pi) / segments\n"
    "        if (mode == \"line\") {\n"
    "            var vertices = []\n"
    "            for (i in 0 .. segments) {\n"
    "                var angle = step * i\n"
    "                vertices.insert(-1, x + angle.sin * radius)\n"
    "                vertices.insert(-1, y + angle.cos * radius)\n"
    "            }\n"
    "            Canvas.polyline(vertices, color)\n"
    "        } else {\n"
    "            var vertices = []\n"
    "            vertices.insert(-1, x)\n"
    "            vertices.insert(-1, y)\n"
    "            for (i in 0 .. segments) {\n"
    "                var angle = step * i\n"
    "                vertices.insert(-1, x + angle.sin * radius)\n"
    "                vertices.insert(-1, y + angle.cos * radius)\n"
    "            }\n"
    "            Canvas.fan(vertices, color)\n"
    "        }\n"
    "    }\n"
    "\n"
    "}\n"
    "\n"
    "class BankBatch {\n"
    "\n"
    "    construct new(bank) {\n"
    "        _bank = bank\n"
    "        _batch = []\n"
    "    }\n"
    "\n"
    "    push(cellId, x, y) {\n"
    "        _batch.add(BankBatchEntry.new(cellId, x, y, 0.0, 1.0, 1.0, 0))\n"
    "    }\n"
    "\n"
    "    push(cellId, x, y, r) {\n"
    "        _batch.add(BankBatchEntry.new(cellId, x, y, r, 1.0, 1.0, 0))\n"
    "    }\n"
    "\n"
    "    push(cellId, x, y, r, sx, sy) {\n"
    "        _batch.add(BankBatchEntry.new(cellId, x, y, r, sx, sy, 0))\n"
    "    }\n"
    "\n"
    "    push(cellId, x, y, r, sx, sy, priority) {\n"
    "        var item = BankBatchEntry.new(cellId, x, y, r, sx, sy, priority)\n"
    "\n"
    "        var index = _batch.count\n"
    "        for (i in 0 ... _batch.count) {\n"
    "            var other = _batch[i]\n"
    "            if (item.priority <= other.priority) {\n"
    "                index = i\n"
    "                break\n"
    "            }\n"
    "        }\n"
    "        _batch.insert(index, item)\n"
    "    }\n"
    "\n"
    "    clear() {\n"
    "        _batch.clear()\n"
    "    }\n"
    "\n"
    "    blit() {\n"
    "        for (item in _batch) {\n"
    "            item.blit(_bank)\n"
    "        }\n"
    "    }\n"
    "\n"
    "}\n"
    "\n"
    "class BankBatchEntry {\n"
    "\n"
    "    construct new(cellId, x, y, r, sx, sy, priority) {\n"
    "        _cellId = cellId\n"
    "        _cameraX = x\n"
    "        _cameraY = y\n"
    "        _r = r\n"
    "        _sx = sx\n"
    "        _sy = sy\n"
    "        _priority = priority\n"
    "    }\n"
    "\n"
    "    priority {\n"
    "        return _priority\n"
    "    }\n"
    "\n"
    "    blit(bank) {\n"
    "        bank.blit(_cellId, _cameraX, _cameraY, _r, _sx, _sy)\n"
    "    }\n"
    "\n"
    "}\n"
;

typedef struct _Font_Class_t {
    // char pathfile[PATH_FILE_MAX];
    // bool loaded;
    GL_Font_t font;
    bool is_default;
} Font_Class_t;

typedef struct _Bank_Class_t { // TODO: rename to `Sheet`?
    // char pathfile[PATH_FILE_MAX];
    // bool loaded;
    int cell_width, cell_height;
    GL_Point_t origin;
    GL_Texture_t atlas;
    GL_Quad_t *quads;
} Bank_Class_t;

void graphics_bank_allocate(WrenVM *vm)
{
    const char *file = wrenGetSlotString(vm, 1);
    int cell_width = (int)wrenGetSlotDouble(vm, 2);
    int cell_height = (int)wrenGetSlotDouble(vm, 3);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Bank.new() -> %s, %d, %d", file, cell_width, cell_height);
#endif

    Bank_Class_t *instance = (Bank_Class_t *)wrenSetSlotNewForeign(vm, 0, 0, sizeof(Bank_Class_t)); // `0, 0` since we are in the allocate callback.

    Environment_t *environment = (Environment_t *)wrenGetUserData(vm);

    char pathfile[PATH_FILE_MAX] = {};
    strcpy(pathfile, environment->base_path);
    strcat(pathfile, file + 2);

    GL_Texture_t atlas;
    GL_texture_load(&atlas, pathfile, to_indexed_atlas_callback, (void *)&environment->display->palette);
    Log_write(LOG_LEVELS_DEBUG, "<GRAPHICS> bank '%s' allocated as #%p", pathfile, instance);

    *instance = (Bank_Class_t){
            .atlas = atlas,
            .quads = GL_texture_quads(&atlas, cell_width, cell_height),
            .cell_width = cell_width,
            .cell_height = cell_height,
            .origin = (GL_Point_t){ (GLfloat)cell_width * 0.5f, (GLfloat)cell_height * 0.5f } // Rotate along center
        };
}

void graphics_bank_finalize(void *userData, void *data)
{
    Bank_Class_t *instance = (Bank_Class_t *)data;

    GL_texture_delete(&instance->atlas);
    Memory_free(instance->quads);
    Log_write(LOG_LEVELS_DEBUG, "<GRAPHICS> bank #%p finalized", instance);

    *instance = (Bank_Class_t){};
}

void graphics_bank_cell_width_get(WrenVM *vm)
{
    const Bank_Class_t *instance = (const Bank_Class_t *)wrenGetSlotForeign(vm, 0);

    wrenSetSlotDouble(vm, 0, instance->cell_width);
}

void graphics_bank_cell_height_get(WrenVM *vm)
{
    const Bank_Class_t *instance = (const Bank_Class_t *)wrenGetSlotForeign(vm, 0);

    wrenSetSlotDouble(vm, 0, instance->cell_height);
}

void graphics_bank_blit_call3(WrenVM *vm)
{
    int cell_id = (int)wrenGetSlotDouble(vm, 1);
    double x = wrenGetSlotDouble(vm, 2);
    double y = wrenGetSlotDouble(vm, 3);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Bank.blit() -> %d, %d, %d", cell_id, x, y);
#endif

    const Bank_Class_t *instance = (const Bank_Class_t *)wrenGetSlotForeign(vm, 0);

//    Environment_t *environment = (Environment_t *)wrenGetUserData(vm);

    double bank_width = (double)instance->cell_width;
    double bank_height = (double)instance->cell_height;

    int dx = (int)((GLfloat)x - instance->origin.x);
    int dy = (int)((GLfloat)y - instance->origin.y);

    GL_Quad_t destination = (GL_Quad_t){ (GLfloat)dx, (GLfloat)dy, (GLfloat)dx + (GLfloat)bank_width, (GLfloat)dy + (GLfloat)bank_height };

    GL_texture_blit_fast(&instance->atlas, instance->quads[cell_id], destination, (GL_Color_t){ 255, 255, 255, 255 });
}

void graphics_bank_blit_call5(WrenVM *vm)
{
    int cell_id = (int)wrenGetSlotDouble(vm, 1);
    double x = wrenGetSlotDouble(vm, 2);
    double y = wrenGetSlotDouble(vm, 3);
    double scale_x = wrenGetSlotDouble(vm, 4);
    double scale_y = wrenGetSlotDouble(vm, 5);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Bank.blit() -> %d, %d, %d, %.3f, %.3f", cell_id, x, y, scale_x, scale_y);
#endif

    const Bank_Class_t *instance = (const Bank_Class_t *)wrenGetSlotForeign(vm, 0);

//    Environment_t *environment = (Environment_t *)wrenGetUserData(vm);

#ifdef __NO_MIRRORING__
    double width = (double)instance->cell_width * fabs(scale_x);
    double height = (double)instance->cell_height * fabs(scale_y);
#else
    double width = (double)instance->cell_width * scale_x; // The sign controls the mirroring.
    double height = (double)instance->cell_height * scale_y;
#endif

    int dx = (int)((GLfloat)x - instance->origin.x);
    int dy = (int)((GLfloat)y - instance->origin.y);

    GL_Quad_t destination = (GL_Quad_t){ (GLfloat)dx, (GLfloat)dy, (GLfloat)dx + (GLfloat)width, (GLfloat)dy + (GLfloat)height };

#ifndef __NO_MIRRORING__
    if (width < 0.0) { // Compensate for mirroring!
        destination.x0 -= width;
        destination.x1 -= width;
    }
    if (height < 0.0) {
        destination.y0 -= height;
        destination.y1 -= height;
    }
#endif

    GL_texture_blit_fast(&instance->atlas, instance->quads[cell_id], destination, (GL_Color_t){ 255, 255, 255, 255 });
}

void graphics_bank_blit_call6(WrenVM *vm)
{
    int cell_id = (int)wrenGetSlotDouble(vm, 1);
    double x = wrenGetSlotDouble(vm, 2);
    double y = wrenGetSlotDouble(vm, 3);
    double rotation = wrenGetSlotDouble(vm, 4);
    double scale_x = wrenGetSlotDouble(vm, 5);
    double scale_y = wrenGetSlotDouble(vm, 6);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Bank.blit() -> %d, %d, %d, %.3f, %.3f, %.3f", cell_id, x, y, rotation, scale_x, scale_y);
#endif

    const Bank_Class_t *instance = (const Bank_Class_t *)wrenGetSlotForeign(vm, 0);

//    Environment_t *environment = (Environment_t *)wrenGetUserData(vm);

#ifdef __NO_MIRRORING__
    double width = (double)instance->cell_width * fabs(scale_x);
    double height = (double)instance->cell_height * fabs(scale_y);
#else
    double width = (double)instance->cell_width * scale_x; // The sign controls the mirroring.
    double height = (double)instance->cell_height * scale_y;
#endif

    int dx = (int)((GLfloat)x - instance->origin.x);
    int dy = (int)((GLfloat)y - instance->origin.y);

    GL_Quad_t destination = (GL_Quad_t){ (GLfloat)dx, (GLfloat)dy, (GLfloat)dx + (GLfloat)width, (GLfloat)dy + (GLfloat)height };

#ifndef __NO_MIRRORING__
    if (width < 0.0) { // Compensate for mirroring!
        destination.x0 -= width;
        destination.x1 -= width;
    }
    if (height < 0.0) {
        destination.y0 -= height;
        destination.y1 -= height;
    }
#endif

    GL_texture_blit(&instance->atlas, instance->quads[cell_id], destination, instance->origin, rotation, (GL_Color_t){ 255, 255, 255, 255 });
}

void graphics_font_allocate(WrenVM *vm)
{
    const char *file = wrenGetSlotString(vm, 1);
    int glyph_width = (int)wrenGetSlotDouble(vm, 2);
    int glyph_height = (int)wrenGetSlotDouble(vm, 3);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Font.new() -> %s, %d, %d", file, glyph_width, glyph_height);
#endif

    Font_Class_t *instance = (Font_Class_t *)wrenSetSlotNewForeign(vm, 0, 0, sizeof(Font_Class_t)); // `0, 0` since we are in the allocate callback.

    Environment_t *environment = (Environment_t *)wrenGetUserData(vm);

    if (strcasecmp(file, "default") == 0) {
        Log_write(LOG_LEVELS_DEBUG, "<GRAPHICS> default font allocated");

        *instance = (Font_Class_t){
                .font = *GL_font_default(),
                .is_default = true
            };
        return;
    }

    char pathfile[PATH_FILE_MAX] = {};
    strcpy(pathfile, environment->base_path);
    strcat(pathfile, file + 2);

    GL_Font_t font;
    GL_font_load(&font, pathfile, glyph_width, glyph_height);
    Log_write(LOG_LEVELS_DEBUG, "<GRAPHICS> font '%s' allocated as #%p", pathfile, instance);

    *instance = (Font_Class_t){
            .font = font,
            .is_default = false
        };
}

void graphics_font_finalize(void *userData, void *data)
{
    Font_Class_t *instance = (Font_Class_t *)data;

    if (!instance->is_default) {
        GL_font_delete(&instance->font);
        Log_write(LOG_LEVELS_DEBUG, "<GRAPHICS> font #%p finalized", instance);
    }

    *instance = (Font_Class_t){};
}

void graphics_font_write_call6(WrenVM *vm) // foreign text(text, color, scale, align)
{
    const char *text = wrenGetSlotString(vm, 1);
    double x = wrenGetSlotDouble(vm, 2);
    double y = wrenGetSlotDouble(vm, 3);
    int color = (int)wrenGetSlotDouble(vm, 4);
    double scale = wrenGetSlotDouble(vm, 5);
    const char *align = wrenGetSlotString(vm, 6);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Font.write() -> %s, %d, %d, %d, %d, %s", text, x, y, color, scale, align);
#endif

    const Font_Class_t *instance = (const Font_Class_t *)wrenGetSlotForeign(vm, 0);

//    Environment_t *environment = (Environment_t *)wrenGetUserData(vm);

    GL_Size_t size = GL_font_measure(&instance->font, text, scale);

    int dx, dy; // Always pixel-aligned positions.
    if (strcmp(align, "left") == 0) {
        dx = (int)x;
        dy = (int)y;
    } else
    if (strcmp(align, "center") == 0) {
        dx = (int)(x - (size.width * 0.5f));
        dy = (int)y;
    } else
    if (strcmp(align, "right") == 0) {
        dx = (int)(x - size.width);
        dy = (int)y;
    } else {
        dx = (int)x;
        dy = (int)y;
    }
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Font.write() -> %d, %d, %d", width, dx, dy);
#endif

    GL_font_write(&instance->font, text, (GL_Point_t){ (GLfloat)dx, (GLfloat)dy }, (GLfloat)scale, (GL_Color_t){ color, color, color, 255 });
}

void graphics_canvas_width_get(WrenVM *vm)
{
    Environment_t *environment = (Environment_t *)wrenGetUserData(vm);

    wrenSetSlotDouble(vm, 0, environment->display->configuration.width);
}

void graphics_canvas_height_get(WrenVM *vm)
{
    Environment_t *environment = (Environment_t *)wrenGetUserData(vm);

    wrenSetSlotDouble(vm, 0, environment->display->configuration.height);
}

void graphics_canvas_palette_get(WrenVM *vm)
{
    Environment_t *environment = (Environment_t *)wrenGetUserData(vm);
    
    GL_Palette_t *palette = &environment->display->palette;

    int slots = wrenGetSlotCount(vm);
#ifdef __DEBUG_VM_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "<GRAPHICS> currently #%d slot(s) available, asking for an additional slot", slots);
#endif
    const int aux_slot_id = slots;
    wrenEnsureSlots(vm, aux_slot_id + 1); // Ask for an additional temporary slot.

#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.palette() -> %d", count);
#endif
    wrenSetSlotNewList(vm, 0); // Create a new list in the return value.

    for (size_t i = 0; i < palette->count; ++i) {
        char argb[12] = {};
        GL_palette_format_color(argb, palette->colors[i]);
        wrenSetSlotString(vm, aux_slot_id, argb);

        wrenInsertInList(vm, 0, i, aux_slot_id);
    }
}

void graphics_canvas_palette_set(WrenVM *vm)
{
    WrenType type = wrenGetSlotType(vm, 1);

    GL_Palette_t palette = {};

    if (type == WREN_TYPE_STRING) { // Predefined palette!
        const char *id = wrenGetSlotString(vm, 1);
        const GL_Palette_t *predefined_palette = graphics_palettes_find(id);
        if (predefined_palette != NULL) {
            palette = *predefined_palette;

            Log_write(LOG_LEVELS_DEBUG, "<GRAPHICS> setting predefined palette '%s' w/ %d color(s)", id, predefined_palette->count);
        } else {
            Log_write(LOG_LEVELS_WARNING, "<GRAPHICS> unknown predefined palette w/ id '%s'", id);
        }
    } else
    if (type == WREN_TYPE_LIST) { // User supplied palette.
        palette.count = wrenGetListCount(vm, 1);
        Log_write(LOG_LEVELS_DEBUG, "<GRAPHICS> setting custom palette of #%d color(s)", palette.count);

        if (palette.count > GL_MAX_PALETTE_COLORS) {
            Log_write(LOG_LEVELS_WARNING, "<GRAPHICS> palette has too many colors (%d) - clamping", palette.count);
            palette.count = MAX_PALETTE_COLORS;
        }

        int slots = wrenGetSlotCount(vm);
#ifdef __DEBUG_VM_CALLS__
        Log_write(LOG_LEVELS_DEBUG, "<GRAPHICS> currently #%d slot(s) available, asking for an additional slot", slots);
#endif
        const int aux_slot_id = slots;
        wrenEnsureSlots(vm, aux_slot_id + 1); // Ask for an additional temporary slot.

#ifdef __DEBUG_API_CALLS__
        Log_write(LOG_LEVELS_DEBUG, "Canvas.palette() -> %d", count);
#endif
        for (size_t i = 0; i < palette.count; ++i) {
            wrenGetListElement(vm, 1, i, aux_slot_id);

            const char *argb = wrenGetSlotString(vm, aux_slot_id);
            palette.colors[i] = GL_palette_parse_color(argb);
        }
    } else { 
        Log_write(LOG_LEVELS_ERROR, "<GRAPHICS> wrong palette type, need to be string or list");
    }

    if (palette.count == 0) {
        return;
    }

    Environment_t *environment = (Environment_t *)wrenGetUserData(vm);
    
    Display_palette(environment->display, &palette);
}

// When drawing points and lines we need to ensure to be in mid-pixel coordinates, according to
// the "diamond exit rule" in OpenGL's rasterization. Also, the ending pixel of a line segment
// is not drawn to avoid lighting a pixel twice in a loop.
//
// http://glprogramming.com/red/appendixg.html#name1
void graphics_canvas_points_call2(WrenVM *vm)
{
    int vertices = wrenGetListCount(vm, 1);
    int color = (int)wrenGetSlotDouble(vm, 2);

    int slots = wrenGetSlotCount(vm);
#ifdef __DEBUG_VM_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "<GRAPHICS> currently #%d slot(s) available, asking for an additional slot", slots);
#endif
    const int aux_slot_id = slots;
    wrenEnsureSlots(vm, aux_slot_id + 1); // Ask for an additional temporary slot.

#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.polygon(%d, %d, %d)", mode, color, vertices);
#endif

    const size_t count = vertices / 2;
    if (count == 0) {
        Log_write(LOG_LEVELS_INFO, "<GRAPHICS> points as no vertices");
        return;
    }

    GL_Point_t points[count];
    for (size_t i = 0; i < count; ++i) {
        wrenGetListElement(vm, 1, (i * 2), aux_slot_id);
        double x = wrenGetSlotDouble(vm, aux_slot_id);
        wrenGetListElement(vm, 1, (i * 2) + 1, aux_slot_id);
        double y = wrenGetSlotDouble(vm, aux_slot_id);

        points[i] = (GL_Point_t){
                .x = (GLfloat)x + 0.375f, .y = (GLfloat)y + 0.375f
            };
    }

    GL_primitive_points(points, count, (GL_Color_t){ color, color, color, 255 });
}

void graphics_canvas_polyline_call2(WrenVM *vm)
{
    int vertices = wrenGetListCount(vm, 1);
    int color = (int)wrenGetSlotDouble(vm, 2);

    int slots = wrenGetSlotCount(vm);
#ifdef __DEBUG_VM_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "<GRAPHICS> currently #%d slot(s) available, asking for an additional slot", slots);
#endif
    const int aux_slot_id = slots;
    wrenEnsureSlots(vm, aux_slot_id + 1); // Ask for an additional temporary slot.

#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.polygon(%d, %d, %d)", mode, color, vertices);
#endif

    const size_t count = vertices / 2;
    if (count == 0) {
        Log_write(LOG_LEVELS_INFO, "<GRAPHICS> lines as no vertices");
        return;
    }

    GL_Point_t points[count];
    for (size_t i = 0; i < count; ++i) {
        wrenGetListElement(vm, 1, (i * 2), aux_slot_id);
        double x = wrenGetSlotDouble(vm, aux_slot_id);
        wrenGetListElement(vm, 1, (i * 2) + 1, aux_slot_id);
        double y = wrenGetSlotDouble(vm, aux_slot_id);

        points[i] = (GL_Point_t){
                .x = (GLfloat)x + 0.375f, .y = (GLfloat)y + 0.375f
            };
    }

    GL_primitive_polyline(points, count, (GL_Color_t){ color, color, color, 255 });
}

void graphics_canvas_strip_call2(WrenVM *vm)
{
    int vertices = wrenGetListCount(vm, 1);
    int color = (int)wrenGetSlotDouble(vm, 2);

    int slots = wrenGetSlotCount(vm);
#ifdef __DEBUG_VM_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "<GRAPHICS> currently #%d slot(s) available, asking for an additional slot", slots);
#endif
    const int aux_slot_id = slots;
    wrenEnsureSlots(vm, aux_slot_id + 1); // Ask for an additional temporary slot.

#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.polygon(%d, %d, %d)", mode, color, vertices);
#endif

    const size_t count = vertices / 2;
    if (count == 0) {
        Log_write(LOG_LEVELS_INFO, "<GRAPHICS> strip as no vertices");
        return;
    }

    GL_Point_t points[count];
    for (size_t i = 0; i < count; ++i) {
        wrenGetListElement(vm, 1, (i * 2), aux_slot_id);
        double x = wrenGetSlotDouble(vm, aux_slot_id);
        wrenGetListElement(vm, 1, (i * 2) + 1, aux_slot_id);
        double y = wrenGetSlotDouble(vm, aux_slot_id);

        points[i] = (GL_Point_t){
                .x = (GLfloat)x, .y = (GLfloat)y
            };
    }

    GL_primitive_strip(points, count, (GL_Color_t){ color, color, color, 255 });
}

void graphics_canvas_fan_call2(WrenVM *vm)
{
    int vertices = wrenGetListCount(vm, 1);
    int color = (int)wrenGetSlotDouble(vm, 2);

    int slots = wrenGetSlotCount(vm);
#ifdef __DEBUG_VM_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "<GRAPHICS> currently #%d slot(s) available, asking for an additional slot", slots);
#endif
    const int aux_slot_id = slots;
    wrenEnsureSlots(vm, aux_slot_id + 1); // Ask for an additional temporary slot.

#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.polygon(%d, %d, %d)", mode, color, vertices);
#endif

    const size_t count = vertices / 2;
    if (count == 0) {
        Log_write(LOG_LEVELS_INFO, "<GRAPHICS> fan as no vertices");
        return;
    }

    GL_Point_t points[count];
    for (size_t i = 0; i < count; ++i) {
        wrenGetListElement(vm, 1, (i * 2), aux_slot_id);
        double x = wrenGetSlotDouble(vm, aux_slot_id);
        wrenGetListElement(vm, 1, (i * 2) + 1, aux_slot_id);
        double y = wrenGetSlotDouble(vm, aux_slot_id);

        points[i] = (GL_Point_t){
                .x = (GLfloat)x, .y = (GLfloat)y
            };
    }

    GL_primitive_fan(points, count, (GL_Color_t){ color, color, color, 255 });
}
