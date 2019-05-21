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
#include "graphics/palettes.h"
#include "graphics/shaders.h"

#include <math.h>
#include <string.h>

#ifdef __EXPLICIT_SIGNUM__
static inline double fsgn(double value)
{
    return (value < 0.0) ? -1.0 : ((value > 0.0) ? 1.0 : value); // On -0.0, +NaN, -NaN, it returns -0.0, +NaN, -NaN
}
#else
static inline double fsgn(double value)
{
    return (0.0 < value) - (value < 0.0); // No cache miss due to branches.
}
#endif

#define DEFAULT_FONT_SIZE   10.0

static void pixel(const GL_Point_t position, const GL_Color_t color)
{

}
static void polygon(const GL_Point_t *point, const size_t count, const GL_Color_t color, bool filled)
{
    
}
static void circle(const GL_Point_t center, const GLfloat radius, const GL_Color_t color, bool filled)
{
    
}
static GLfloat measure(const Font_t *font, const char *text, const GLfloat size)
{
    return 1.0f;
}
static void write(const Font_t *font, const char *text, const GL_Point_t position, const GLfloat size, const GL_Color_t color)
{
    
}

const char graphics_wren[] =
    "foreign class Bank {\n"
    "\n"
    "    construct new(file, cellWidth, cellHeight) {}\n"
    "\n"
    "    foreign cellWidth\n"
    "    foreign cellHeight\n"
    "\n"
    "    blit(cellId, x, y) {\n"
    "        blit(cellId, x, y, 0.0, 1.0, 1.0)\n"
    "    }\n"
    "    blit(cellId, x, y, r) {\n"
    "        blit(cellId, x, y, r, 1.0, 1.0)\n"
    "    }\n"
    "    foreign blit(cellId, x, y, r, sx, sy)\n"
    "\n"
    "}\n"
    "\n"
    "foreign class Font {\n"
    "\n"
    "    construct new(file) {}\n"
    "\n"
    "    static default { Font.new(\"default\") }\n"
    "\n"
    "    foreign write(text, x, y, color, size, align)\n"
    "\n"
    "}\n"
    "\n"
    "foreign class Canvas {\n"
    "\n"
    "    foreign static width\n"
    "    foreign static height\n"
    "    foreign static palette(colors)\n"
    "    foreign static shader(index, code)\n"
    "\n"
    "    foreign static point(x, y, color)\n"
    "    foreign static polygon(mode, vertices, color)\n"
    "    foreign static circle(mode, x, y, radius, color)\n"
    "\n"
    "    static line(x0, y0, x1, y1, color) {\n"
    "        polygon(\"line\", [ x0, y0, x1, y1 ], color)\n"
    "    }\n"
    "    static triangle(mode, x0, y0, x1, y1, x2, y2, color) {\n"
    "        polygon(mode, [ x0, y0, x1, y1, x2, y2, x0, y0 ], color)\n"
    "    }\n"
    "    static rectangle(mode, x, y, width, height, color) {\n"
    "        var offset = mode == \"line\" ? 1 : 0\n"
    "        var left = x\n"
    "        var top = y\n"
    "        var right = left + width - offset\n"
    "        var bottom = top + height - offset\n"
    "        polygon(mode, [ left, top, left, bottom, right, bottom, right, top, left, top ], color)\n"
    "    }\n"
    "    static square(mode, x, y, size, color) {\n"
    "        rectangle(mode, x, y, size, size, color)\n"
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

void graphics_bank_allocate(WrenVM *vm)
{
    const char *file = wrenGetSlotString(vm, 1);
    int cell_width = (int)wrenGetSlotDouble(vm, 2);
    int cell_height = (int)wrenGetSlotDouble(vm, 3);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Bank.new() -> %s, %d, %d", file, cell_width, cell_height);
#endif

    Bank_t *bank = (Bank_t *)wrenSetSlotNewForeign(vm, 0, 0, sizeof(Bank_t)); // `0, 0` since we are in the allocate callback.

    Environment_t *environment = (Environment_t *)wrenGetUserData(vm);

    char pathfile[PATH_FILE_MAX] = {};
    strcpy(pathfile, environment->base_path);
    strcat(pathfile, file + 2);

    *bank = load_bank(pathfile, cell_width, cell_height, &environment->display->palette);
}

void graphics_bank_finalize(void *userData, void *data)
{
    Bank_t *bank = (Bank_t *)data;
    unload_bank(bank);
}

void graphics_bank_cell_width_get(WrenVM *vm)
{
    const Bank_t *bank = (const Bank_t *)wrenGetSlotForeign(vm, 0);

    wrenSetSlotDouble(vm, 0, bank->cell_width);
}

void graphics_bank_cell_height_get(WrenVM *vm)
{
    const Bank_t *bank = (const Bank_t *)wrenGetSlotForeign(vm, 0);

    wrenSetSlotDouble(vm, 0, bank->cell_height);
}

void graphics_bank_blit_call6(WrenVM *vm)
{
    int cell_id = (int)wrenGetSlotDouble(vm, 1);
    int x = (int)wrenGetSlotDouble(vm, 2);
    int y = (int)wrenGetSlotDouble(vm, 3);
    double rotation = wrenGetSlotDouble(vm, 4);
    double scale_x = wrenGetSlotDouble(vm, 5);
    double scale_y = wrenGetSlotDouble(vm, 6);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Bank.blit() -> %d, %d, %d, %.3f, %.3f, %.3f", cell_id, x, y, rotation, scale_x, scale_y);
#endif

    const Bank_t *bank = (const Bank_t *)wrenGetSlotForeign(vm, 0);

    if (!bank->loaded) {
        Log_write(LOG_LEVELS_ERROR, "<GRAPHICS> bank now loaded, can't draw cell");
        return;
    }

//    Environment_t *environment = (Environment_t *)wrenGetUserData(vm);

    int bank_position = cell_id * bank->cell_width;
    int bank_x = bank_position % bank->atlas.width;
    int bank_y = (bank_position / bank->atlas.width) * bank->cell_height;
    double bank_width = (double)bank->cell_width * fsgn(scale_x); // The sign controls the mirroring.
    double bank_height = (double)bank->cell_height * fsgn(scale_y);

    double width = (double)bank->cell_width * fabs(scale_x);
    double height = (double)bank->cell_height * fabs(scale_y);
    double half_width = width * 0.5f; // Offset to compensate for origin (rotation)
    double half_height = height * 0.5f;

    GL_Rectangle_t source = (GL_Rectangle_t){ (GLfloat)bank_x, (GLfloat)bank_y, (GLfloat)bank_width, (GLfloat)bank_height };
    GL_Rectangle_t destination = (GL_Rectangle_t){ (GLfloat)x + (GLfloat)half_width, (GLfloat)y + (GLfloat)half_height, (GLfloat)width, (GLfloat)height };

    GL_draw_texture(&bank->atlas, source, destination, bank->origin, rotation, (GL_Color_t){ 255, 255, 255, 255 });
}

void graphics_font_allocate(WrenVM *vm)
{
    const char *file = wrenGetSlotString(vm, 1);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Font.new() -> %s", file);
#endif

    Font_t *font = (Font_t *)wrenSetSlotNewForeign(vm, 0, 0, sizeof(Font_t)); // `0, 0` since we are in the allocate callback.

    if (strcmp(file, "default") == 0) {
        *font = (Font_t){
                .loaded = true,
//                .font = GetFontDefault()
            };
        return;
    }

    Environment_t *environment = (Environment_t *)wrenGetUserData(vm);

    char pathfile[PATH_FILE_MAX] = {};
    strcpy(pathfile, environment->base_path);
    strcat(pathfile, file + 2);

    *font = load_font(pathfile);
}

void graphics_font_finalize(void *userData, void *data)
{
    Font_t *font = (Font_t *)data;
    unload_font(font);
}

void graphics_font_write_call6(WrenVM *vm) // foreign text(text, color, size, align)
{
    const char *text = wrenGetSlotString(vm, 1);
    double x = wrenGetSlotDouble(vm, 2);
    double y = wrenGetSlotDouble(vm, 3);
    int color = (int)wrenGetSlotDouble(vm, 4);
    double size = wrenGetSlotDouble(vm, 5);
    const char *align = wrenGetSlotString(vm, 6);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Font.write() -> %s, %d, %d, %d, %d, %s", text, x, y, color, size, align);
#endif

    const Font_t *font = (const Font_t *)wrenGetSlotForeign(vm, 0);

//    Environment_t *environment = (Environment_t *)wrenGetUserData(vm);

    int width = measure(font, text, size);

    int dx = x, dy = y;
    if (strcmp(align, "left") == 0) {
        dx = x;
        dy = y;
    } else
    if (strcmp(align, "center") == 0) {
        dx = x - (width / 2);
        dy = y;
    } else
    if (strcmp(align, "right") == 0) {
        dx = x - width;
        dy = y;
    }
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Font.write() -> %d, %d, %d", width, dx, dy);
#endif

    if (!font->loaded) {
        return;
    }

    // Spacing is proportional to default font size.
    if (size < DEFAULT_FONT_SIZE) {
        size = DEFAULT_FONT_SIZE;
    }

    write(font, text, (GL_Point_t){ dx, dy }, size, (GL_Color_t){ color, color, color, 255 });
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

void graphics_canvas_palette_call1(WrenVM *vm)
{
    WrenType type = wrenGetSlotType(vm, 1);

    GL_Palette_t palette = {};

    if (type == WREN_TYPE_STRING) { // Predefined palette!
        const char *id = wrenGetSlotString(vm, 1);
        const GL_Palette_t *predefined_palette = graphics_palettes_find(id);
        if (predefined_palette != NULL) {
            palette.count = predefined_palette->count;
            memcpy(palette.colors, predefined_palette->colors, sizeof(GL_Color_t) * predefined_palette->count);

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
        Log_write(LOG_LEVELS_DEBUG, "Currently #%d slot(s) available, asking for an additional slot", slots);
#endif
        const int aux_slot_id = slots;
        wrenEnsureSlots(vm, aux_slot_id + 1); // Ask for an additional temporary slot.

#ifdef __DEBUG_API_CALLS__
        Log_write(LOG_LEVELS_DEBUG, "Canvas.palette() -> %d", count);
#endif
        for (size_t i = 0; i < palette.count; ++i) {
            wrenGetListElement(vm, 1, i, aux_slot_id);

            const char *argb = wrenGetSlotString(vm, aux_slot_id);
            palette.colors[i] = GL_parse_color(argb);
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

void graphics_canvas_shader_call2(WrenVM *vm)
{
    int index = (int)wrenGetSlotDouble(vm, 1);
    const char *code = wrenGetSlotString(vm, 2);

    int shader_index = ((index > SHADERS_COUNT - 1) ? SHADERS_COUNT - 2 : index) + 1; // Skip palette shader.
    const char *shader_code = graphics_shaders_find(code);

    if (shader_code != NULL) { // Predefined shader.
        Log_write(LOG_LEVELS_DEBUG, "<GRAPHICS> setting predefined shader '%s' for index #%d", code, shader_index);
    } else {
        shader_code = code;

        Log_write(LOG_LEVELS_DEBUG, "<GRAPHICS> setting user-defined shader '%s' for index #%d", code, shader_index);
    }

    Environment_t *environment = (Environment_t *)wrenGetUserData(vm);
    
    Display_shader(environment->display, shader_index, shader_code);
}

void graphics_canvas_point_call3(WrenVM *vm)
{
    double x = wrenGetSlotDouble(vm, 1);
    double y = wrenGetSlotDouble(vm, 2);
    int color = (int)wrenGetSlotDouble(vm, 3);

    pixel((GL_Point_t){ x, y}, (GL_Color_t){ color, color, color, 255 });
}

void graphics_canvas_polygon_call3(WrenVM *vm)
{
    const char *mode = wrenGetSlotString(vm, 1);
    int vertices = wrenGetListCount(vm, 2);
    int color = (int)wrenGetSlotDouble(vm, 3);

    int slots = wrenGetSlotCount(vm);
#ifdef __DEBUG_VM_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Currently #%d slot(s) available, asking for an additional slot", slots);
#endif
    const int aux_slot_id = slots;
    wrenEnsureSlots(vm, aux_slot_id + 1); // Ask for an additional temporary slot.

#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.polygon(%d, %d, %d)", mode, color, vertices);
#endif

    const size_t count = vertices / 2;
    if (count == 0) {
        Log_write(LOG_LEVELS_INFO, "<GRAPHICS> polygon as no vertices");
        return;
    }

    // When drawing lines we need to ensure to be in mid-pixel coordinates. Also the length of lines are inclusive
    // (and this need to be taken into account for rectangles/squares). This is due to the "diamond exit rule" in
    // OpenGL rasterization.
    //
    // http://glprogramming.com/red/appendixg.html#name1
    double offset = (strcasecmp(mode, "line") == 0) ? 0.5 : 0.0;

    GL_Point_t points[count];
    for (size_t i = 0; i < count; ++i) {
        wrenGetListElement(vm, 2, (i * 2), aux_slot_id);
        double x = wrenGetSlotDouble(vm, aux_slot_id);
        wrenGetListElement(vm, 2, (i * 2) + 1, aux_slot_id);
        double y = wrenGetSlotDouble(vm, aux_slot_id);

        points[i] = (GL_Point_t){
                .x = (GLfloat)(x + offset), .y = (GLfloat)(y + offset) // HAZARD: should cast separately?
            };
    }

    if (strcasecmp(mode, "fill") == 0) {
        polygon(points, count, (GL_Color_t){ color, color, color, 255 }, true);
    } else
    if (strcasecmp(mode, "line") == 0) {
        polygon(points, count, (GL_Color_t){ color, color, color, 255 }, false);
    } else {
        Log_write(LOG_LEVELS_WARNING, "<GRAPHICS> undefined drawing mode for polygon: '%s'", mode);
    }
}

void graphics_canvas_circle_call5(WrenVM *vm)
{
    const char *mode = wrenGetSlotString(vm, 1);
    double x = wrenGetSlotDouble(vm, 2);
    double y = wrenGetSlotDouble(vm, 3);
    double radius =wrenGetSlotDouble(vm, 4);
    int color = (int)wrenGetSlotDouble(vm, 5);

#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.circle(%s, %d, %d, %d, %d)", mode, x, y, radius, color);
#endif

    if (strcasecmp(mode, "fill") == 0) {
        circle((GL_Point_t){ x, y }, (GLfloat)radius, (GL_Color_t){ color, color, color, 255 }, true);
    } else
    if (strcasecmp(mode, "line") == 0) {
        circle((GL_Point_t){ x, y }, (GLfloat)radius, (GL_Color_t){ color, color, color, 255 }, false);
//     } else
//     if (strcmp(mode, "sector") == 0) {
//         DrawCircleSector(x, y, radius, (Color){ color, color, color, 255 });
    } else {
        Log_write(LOG_LEVELS_WARNING, "<GRAPHICS> undefined drawing mode for polygon: '%s'", mode);
    }
}
