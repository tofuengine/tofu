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
#include "../gl/gl.h"
#include "graphics/palettes.h"
#include "graphics/shaders.h"

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
static void palettize_callback(void *parameters, void *data, int width, int height)
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

typedef struct _Font_Class_t {
    // char pathfile[PATH_FILE_MAX];
    // bool loaded;
    GL_Font_t font;
} Font_Class_t;

typedef struct _Bank_Class_t { // TODO: rename to `Sheet`?
    // char pathfile[PATH_FILE_MAX];
    // bool loaded;
    int cell_width, cell_height;
    GL_Point_t origin;
    GL_Texture_t atlas;
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

    GL_Texture_t texture;
    GL_create_texture(&texture, pathfile, palettize_callback, (void *)&environment->display->palette);
    Log_write(LOG_LEVELS_DEBUG, "<GRAPHICS> bank '%s' allocated as #%p", pathfile, instance);

    *instance = (Bank_Class_t){
            .atlas = texture,
            .cell_width = cell_width,
            .cell_height = cell_height,
            .origin = (GL_Point_t){ cell_width * 0.5, cell_height * 0.5} // Rotate along center
        };
}

void graphics_bank_finalize(void *userData, void *data)
{
    Bank_Class_t *instance = (Bank_Class_t *)data;

    GL_delete_texture(&instance->atlas);
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

    const Bank_Class_t *instance = (const Bank_Class_t *)wrenGetSlotForeign(vm, 0);

//    Environment_t *environment = (Environment_t *)wrenGetUserData(vm);

    int bank_position = cell_id * instance->cell_width;
    int bank_x = bank_position % instance->atlas.width;
    int bank_y = (bank_position / instance->atlas.width) * instance->cell_height;
    double bank_width = (double)instance->cell_width * fsgn(scale_x); // The sign controls the mirroring.
    double bank_height = (double)instance->cell_height * fsgn(scale_y);

    double width = (double)instance->cell_width * fabs(scale_x);
    double height = (double)instance->cell_height * fabs(scale_y);
    double half_width = width * 0.5f; // Offset to compensate for origin (rotation)
    double half_height = height * 0.5f;

    GL_Rectangle_t source = (GL_Rectangle_t){ (GLfloat)bank_x, (GLfloat)bank_y, (GLfloat)bank_width, (GLfloat)bank_height };
    GL_Rectangle_t destination = (GL_Rectangle_t){ (GLfloat)x + (GLfloat)half_width, (GLfloat)y + (GLfloat)half_height, (GLfloat)width, (GLfloat)height };

    GL_draw_texture(&instance->atlas, source, destination, instance->origin, rotation, (GL_Color_t){ 255, 255, 255, 255 });
}

void graphics_font_allocate(WrenVM *vm)
{
    const char *file = wrenGetSlotString(vm, 1);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Font.new() -> %s", file);
#endif

    Font_Class_t *instance = (Font_Class_t *)wrenSetSlotNewForeign(vm, 0, 0, sizeof(Font_Class_t)); // `0, 0` since we are in the allocate callback.

    Environment_t *environment = (Environment_t *)wrenGetUserData(vm);

    char pathfile[PATH_FILE_MAX] = {};
    strcpy(pathfile, environment->base_path);
    strcat(pathfile, file + 2);

    GL_Font_t font;
    GL_font_create(&font, pathfile, 16, 16, "ABCDEFG");
    Log_write(LOG_LEVELS_DEBUG, "<GRAPHICS> font '%s' allocated as #%p", pathfile, instance);

    *instance = (Font_Class_t){
            .font = font
        };
}

void graphics_font_finalize(void *userData, void *data)
{
    Font_Class_t *instance = (Font_Class_t *)data;

    GL_font_delete(&instance->font);
    Log_write(LOG_LEVELS_DEBUG, "<GRAPHICS> font #%p finalized", instance);

    *instance = (Font_Class_t){};
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

    const Font_Class_t *instance = (const Font_Class_t *)wrenGetSlotForeign(vm, 0);

//    Environment_t *environment = (Environment_t *)wrenGetUserData(vm);

    GL_Rectangle_t rectangle = GL_font_measure(&instance->font, text, size);

    GLfloat dx = x, dy = y;
    if (strcmp(align, "left") == 0) {
        dx = x;
        dy = y;
    } else
    if (strcmp(align, "center") == 0) {
        dx = x - (rectangle.width / 2);
        dy = y;
    } else
    if (strcmp(align, "right") == 0) {
        dx = x - rectangle.width;
        dy = y;
    }
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Font.write() -> %d, %d, %d", width, dx, dy);
#endif

    GL_font_write(&instance->font, text, (GL_Point_t){ (GLfloat)dx, (GLfloat)dy }, (GLfloat)size, (GL_Color_t){ color, color, color, 255 });
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

    GL_primitive_point((GL_Point_t){ x, y}, (GL_Color_t){ color, color, color, 255 });
}

void graphics_canvas_polygon_call3(WrenVM *vm)
{
    const char *mode = wrenGetSlotString(vm, 1);
    int vertices = wrenGetListCount(vm, 2);
    int color = (int)wrenGetSlotDouble(vm, 3);

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
        GL_primitive_polygon(points, count, (GL_Color_t){ color, color, color, 255 }, true);
    } else
    if (strcasecmp(mode, "line") == 0) {
        GL_primitive_polygon(points, count, (GL_Color_t){ color, color, color, 255 }, false);
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
        GL_primitive_circle((GL_Point_t){ x, y }, (GLfloat)radius, (GL_Color_t){ color, color, color, 255 }, true);
    } else
    if (strcasecmp(mode, "line") == 0) {
        GL_primitive_circle((GL_Point_t){ x, y }, (GLfloat)radius, (GL_Color_t){ color, color, color, 255 }, false);
//     } else
//     if (strcmp(mode, "sector") == 0) {
//         DrawCircleSector(x, y, radius, (Color){ color, color, color, 255 });
    } else {
        Log_write(LOG_LEVELS_WARNING, "<GRAPHICS> undefined drawing mode for polygon: '%s'", mode);
    }
}
