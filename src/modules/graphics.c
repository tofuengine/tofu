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

#include "../environment.h"
#include "../log.h"

#include <raylib/raylib.h>
#include <string.h>

#define DEFAULT_FONT_ID -1

#define UNUSED(x)       (void)(x)

const char graphics_wren[] =
    "foreign class Canvas {\n"
    "\n"
    "    foreign static width\n"
    "    foreign static height\n"
    "    foreign static palette(colors)\n"
    "    foreign static font(font_id, file)\n"
    "    foreign static bank(bank_id, file, width, height)\n"
    "\n"
    "    foreign static defaultFont\n"
    "    foreign static text(font_id, text, x, y, color, size, align)\n"
    "\n"
    "    foreign static point(x, y, color)\n"
    "    foreign static polygon(mode, vertices, color)\n"
    "    foreign static circle(mode, x, y, radius, color)\n"
    "\n"
    "    foreign static sprite(bank_id, sprite_id, x, y, r, sx, sy)\n"
    "\n"
    "    static line(x0, y0, x1, y1, color) {\n"
    "        polygon(\"line\", [ x0, y0, x1, y1 ], color)\n"
    "    }\n"
    "    static triangle(mode, x0, y0, x1, y1, x2, y2, color) {\n"
    "        polygon(mode, [ x0, y0, x1, y1, x2, y2, x0, y0 ], color)\n"
    "    }\n"
    "    static rectangle(mode, x, y, width, height, color) {\n"
    "        var left = x\n"
    "        var top = y\n"
    "        var right = left + width - 1\n"
    "        var bottom = top + height - 1\n"
    "        polygon(mode, [ left, top, left, bottom, right, bottom, right, top, left, top ], color)\n"
    "    }\n"
    "    static square(mode, x, y, size, color) {\n"
    "        var left = x\n"
    "        var top = y\n"
    "        var right = left + size - 1\n"
    "        var bottom = top + size - 1\n"
    "        polygon(mode, [ left, top, left, bottom, right, bottom, right, top, left, top ], color)\n"
    "    }\n"
    "\n"
    "    static sprite(bank_id, sprite_id, x, y) {\n"
    "        sprite(bank_id, sprite_id, x, y, 0.0, 1.0, 1.0)\n"
    "    }\n"
    "    static sprite(bank_id, sprite_id, x, y, r) {\n"
    "        sprite(bank_id, sprite_id, x, y, r, 1.0, 1.0)\n"
    "    }\n"
    "\n"
    "}\n"
;

void graphics_canvas_width(WrenVM *vm)
{
    Environment_t *environment = (Environment_t *)wrenGetUserData(vm);

    wrenSetSlotDouble(vm, 0, environment->display->configuration.width);
}

void graphics_canvas_height(WrenVM *vm)
{
    Environment_t *environment = (Environment_t *)wrenGetUserData(vm);

    wrenSetSlotDouble(vm, 0, environment->display->configuration.height);
}

void graphics_canvas_palette(WrenVM *vm)
{
    Environment_t *environment = (Environment_t *)wrenGetUserData(vm);

    int count = wrenGetListCount(vm, 1);
    if (count > MAX_PALETTE_COLORS) {
        Log_write(LOG_LEVELS_WARNING, "[TOFU] Palette has too many colors (%d) - clamping!", count);
        count = MAX_PALETTE_COLORS;
    }

    int slots = wrenGetSlotCount(vm);
    const int aux_slot_id = slots;
#ifdef DEBUG
    Log_write(LOG_LEVELS_DEBUG, "Currently #%d slot(s) available, asking for additional slot", slots);
#endif
    wrenEnsureSlots(vm, aux_slot_id + 1); // Ask for an additional temporary slot.

#ifdef DEBUG
    Log_write(LOG_LEVELS_DEBUG, "Canvas.palette() -> %d", count);
#endif
    Color palette[MAX_PALETTE_COLORS];
    for (int i = 0; i < count; ++i) {
        wrenGetListElement(vm, 1, i, aux_slot_id);

        Color *color = &palette[i];
        const char *argb = wrenGetSlotString(vm, aux_slot_id);
        char hex[3] = {};
        strncpy(hex, argb    , 2); color->a = strtol(hex, NULL, 16);
        strncpy(hex, argb + 2, 2); color->r = strtol(hex, NULL, 16);
        strncpy(hex, argb + 4, 2); color->g = strtol(hex, NULL, 16);
        strncpy(hex, argb + 6, 2); color->b = strtol(hex, NULL, 16);
    }

    Display_palette(environment->display, palette, count);
}

void graphics_canvas_font(WrenVM *vm)
{
    Environment_t *environment = (Environment_t *)wrenGetUserData(vm);

    int font_id = (int)wrenGetSlotDouble(vm, 1);
    const char *file = wrenGetSlotString(vm, 2);
#ifdef DEBUG
    Log_write(LOG_LEVELS_DEBUG, "Canvas.font() -> %d, %s", font_id, file);
#endif

    char pathfile[PATH_FILE_MAX] = {};
    strcpy(pathfile, environment->base_path);
    strcat(pathfile, file + 2);

    Font font = LoadFont(pathfile);
    Log_write(LOG_LEVELS_DEBUG, "[TOFU] Font '%s' loaded as texture w/ id #%d", pathfile, font.texture.id);

    // TODO: unload font if already in use!

    environment->display->fonts[font_id] = (Font_t){
//            .pathfile = pathfile,
            .loaded = true,
            .font = font
        };
}

void graphics_canvas_bank(WrenVM *vm)
{
    Environment_t *environment = (Environment_t *)wrenGetUserData(vm);

    int bank_id = (int)wrenGetSlotDouble(vm, 1);
    const char *file = wrenGetSlotString(vm, 2);
    int cell_width = (int)wrenGetSlotDouble(vm, 3);
    int cell_height = (int)wrenGetSlotDouble(vm, 4);
#ifdef DEBUG
    Log_write(LOG_LEVELS_DEBUG, "Canvas.bank() -> %d, %s, %d, %d", bank_id, file, cell_width, cell_height);
#endif

    char pathfile[PATH_FILE_MAX] = {};
    strcpy(pathfile, environment->base_path);
    strcat(pathfile, file + 2);

    Image image = LoadImage(pathfile); // Load and remap image colors to the current palette.
    for (int i = 0; i < MAX_PALETTE_COLORS; ++i) {
        ImageColorReplace(&image, environment->display->palette[i], (Color){ i, i, i, 255 });
    }
    Texture2D texture = LoadTextureFromImage(image);
    Log_write(LOG_LEVELS_DEBUG, "[TOFU] Bank '%s' loaded as texture w/ id #%d", pathfile, texture.id);

    // TODO: unload bank if already in use!

    environment->display->banks[bank_id] = (Bank_t){
            .loaded = true,
            .atlas = texture,
            .cell_width = cell_width,
            .cell_height = cell_height
        };
}

void graphics_canvas_defaultFont(WrenVM *vm)
{
    wrenSetSlotDouble(vm, 0, DEFAULT_FONT_ID);
}

void graphics_canvas_text(WrenVM *vm) // foreign static text(font_id, text, color, size, align)
{
    Environment_t *environment = (Environment_t *)wrenGetUserData(vm);

    int font_id = (int)wrenGetSlotDouble(vm, 1);
    const char *text = wrenGetSlotString(vm, 2);
    int x = (int)wrenGetSlotDouble(vm, 3);
    int y = (int)wrenGetSlotDouble(vm, 4);
    int color = (int)wrenGetSlotDouble(vm, 5);
    int size = (int)wrenGetSlotDouble(vm, 6);
    const char *align = wrenGetSlotString(vm, 7);
#ifdef DEBUG
    Log_write(LOG_LEVELS_DEBUG, "Canvas.text() -> %d, %s, %d, %d, %d, %d, %s", font_id, text, x, y, color, size, align);
#endif

    int width = MeasureText(text, size);

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
#ifdef DEBUG
    Log_write(LOG_LEVELS_DEBUG, "Canvas.text() -> %d, %d, %d", width, dx, dy);
#endif

    if (font_id == DEFAULT_FONT_ID) {
        DrawText(text, dx, dy, size, (Color){ color, color, color, 255 });
    }

    const Font_t *font = &environment->display->fonts[font_id];

    if (font->font.texture.id == 0) {
        return;
    }

    // Spacing is proportional to default font size.
    const int defaultFontSize = 10;   // Default Font chars height in pixel
    if (size < defaultFontSize) {
        size = defaultFontSize;
    }
    int spacing = size / defaultFontSize;

    DrawTextEx(font->font, text, (Vector2){ dx, dy }, size, (float)spacing, (Color){ color, color, color, 255 });
}

void graphics_canvas_point(WrenVM *vm)
{
    int x = (int)wrenGetSlotDouble(vm, 1);
    int y = (int)wrenGetSlotDouble(vm, 2);
    int color = (int)wrenGetSlotDouble(vm, 3);

    DrawPixel(x, y, (Color){ color, color, color, 255 });
}

void graphics_canvas_polygon(WrenVM *vm)
{
    const char *mode = wrenGetSlotString(vm, 1);
    int vertices = wrenGetListCount(vm, 2);
    int color = (int)wrenGetSlotDouble(vm, 3);

    int slots = wrenGetSlotCount(vm);
    const int aux_slot_id = slots;
#ifdef DEBUG
    Log_write(LOG_LEVELS_DEBUG, "Currently #%d slot(s) available, asking for additional slot", slots);
#endif
    wrenEnsureSlots(vm, aux_slot_id + 1); // Ask for an additional temporary slot.

#ifdef DEBUG
    Log_write(LOG_LEVELS_DEBUG, "Canvas.polygon(%d, %d, %d)", mode, color, vertices);
#endif

    const int count = vertices / 2;
    if (count == 0) {
        Log_write(LOG_LEVELS_INFO, "[TOFU] Polygon as no vertices");
        return;
    }

    Vector2 points[count];
    for (int i = 0; i < count; ++i) {
        wrenGetListElement(vm, 2, (i * 2), aux_slot_id);
        int x = (int)wrenGetSlotDouble(vm, aux_slot_id);
        wrenGetListElement(vm, 2, (i * 2) + 1, aux_slot_id);
        int y = (int)wrenGetSlotDouble(vm, aux_slot_id);

        points[i] = (Vector2){
                .x = x, .y = y
            };
    }

    if (strcmp(mode, "fill") == 0) {
        DrawPolyEx(points, count, (Color){ color, color, color, 255 });
    } else
    if (strcmp(mode, "line") == 0) {
        DrawPolyExLines(points, count, (Color){ color, color, color, 255 });
    } else {
        Log_write(LOG_LEVELS_WARNING, "[TOFU] Undefined drawing mode for polygon: '%s'", mode);
    }
}

void graphics_canvas_circle(WrenVM *vm)
{
    const char *mode = wrenGetSlotString(vm, 1);
    int x = (int)wrenGetSlotDouble(vm, 2);
    int y = (int)wrenGetSlotDouble(vm, 3);
    float radius = (float)wrenGetSlotDouble(vm, 4);
    int color = (int)wrenGetSlotDouble(vm, 5);

#ifdef DEBUG
    Log_write(LOG_LEVELS_DEBUG, "Canvas.circle(%s, %d, %d, %d, %d)", mode, x, y, radius, color);
#endif

    if (strcmp(mode, "fill") == 0) {
        DrawCircle(x, y, radius, (Color){ color, color, color, 255 });
    } else
    if (strcmp(mode, "line") == 0) {
        DrawCircleLines(x, y, radius, (Color){ color, color, color, 255 });
//     } else
//     if (strcmp(mode, "sector") == 0) {
//         DrawCircleSector(x, y, radius, (Color){ color, color, color, 255 });
    } else {
        Log_write(LOG_LEVELS_WARNING, "[TOFU] Undefined drawing mode for polygon: '%s'", mode);
    }
}

void graphics_canvas_sprite(WrenVM *vm)
{
    Environment_t *environment = (Environment_t *)wrenGetUserData(vm);

    int bank_id = (int)wrenGetSlotDouble(vm, 1);
    int sprite_id = (int)wrenGetSlotDouble(vm, 2);
    int x = (int)wrenGetSlotDouble(vm, 3);
    int y = (int)wrenGetSlotDouble(vm, 4);
    double rotation = wrenGetSlotDouble(vm, 5);
    double scale_x = wrenGetSlotDouble(vm, 6);
    double scale_y = wrenGetSlotDouble(vm, 7);
#ifdef DEBUG
    Log_write(LOG_LEVELS_DEBUG, "Canvas.sprite() -> %d, %d, %d, %d, %.3f, %.3f, %.3f", bank_id, sprite_id, x, y, rotation, scale_x, scale_y);
#endif

    const Bank_t *bank = &environment->display->banks[bank_id];

    if (bank->atlas.id == 0) {
        return;
    }

    int bank_position = sprite_id * bank->cell_width;
    int bank_x = bank_position % bank->atlas.width;
    int bank_y = (bank_position / bank->atlas.width) * bank->cell_height;

    Rectangle sourceRec = { (float)bank_x, (float)bank_y, (float)bank->cell_width, (float)bank->cell_height };
    Rectangle destRec = { x, y, (float)bank->cell_width * scale_x, (float)bank->cell_height * scale_y };
    Vector2 origin = { bank->cell_width / 2.0f, bank->cell_height / 2.0f }; // TODO: make origin configurable.

    DrawTexturePro(bank->atlas, sourceRec, destRec, origin, (float)rotation, (Color){ 255, 255, 255, 255 });
}

