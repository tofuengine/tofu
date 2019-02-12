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

#include <raylib/raylib.h>
#include <string.h>

#include "environment.h"
#include "log.h"

#include "modules/graphics_wren.inc"
#include "modules/events_wren.inc"

#define UNUSED(x)       (void)(x)

static void graphics_canvas_width(WrenVM *vm)
{
    Environment_t *environment = (Environment_t *)wrenGetUserData(vm);

    wrenSetSlotDouble(vm, 0, environment->display->configuration.width);
}

static void graphics_canvas_height(WrenVM *vm)
{
    Environment_t *environment = (Environment_t *)wrenGetUserData(vm);

    wrenSetSlotDouble(vm, 0, environment->display->configuration.height);
}

static void graphics_canvas_palette(WrenVM *vm)
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

static void graphics_canvas_bank(WrenVM *vm)
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

    environment->display->banks[bank_id] = (Bank_t){
            .atlas = texture,
            .cell_width = cell_width,
            .cell_height = cell_height
        };
}

static void graphics_canvas_text(WrenVM *vm) // foreign static text(font_id, text, color, size, align)
{
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

    UNUSED(font_id);

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

    DrawText(text, dx, dy, size, (Color){ color, color, color, 255 });
}

static void graphics_canvas_point(WrenVM *vm)
{
    int x = (int)wrenGetSlotDouble(vm, 1); 
    int y = (int)wrenGetSlotDouble(vm, 2); 
    int color = (int)wrenGetSlotDouble(vm, 3); 

    DrawPixel(x, y, (Color){ color, color, color, 255 });
}

static void graphics_canvas_polygon(WrenVM *vm)
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

static void graphics_canvas_sprite(WrenVM *vm)
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

    Rectangle sourceRec = { sprite_id * bank->cell_width, 0.0f, (float)bank->cell_width, (float)bank->cell_height };
    Rectangle destRec = { x, y, (float)bank->cell_width * scale_x, (float)bank->cell_height * scale_y };
    Vector2 origin = { bank->cell_width / 2.0f, bank->cell_height / 2.0f };

    DrawTexturePro(bank->atlas, sourceRec, destRec, origin, (float)rotation, (Color){ 255, 255, 255, 255 });
}

static void events_input_iskeydown(WrenVM *vm)
{
    // Environment_t *environment = (Environment_t *)wrenGetUserData(vm);

    int key = (int)wrenGetSlotDouble(vm, 1);
    bool is_down = IsKeyDown(key);
    wrenSetSlotBool(vm, 0, is_down == true);
}

static void events_input_iskeyup(WrenVM *vm)
{
    // Environment_t *environment = (Environment_t *)wrenGetUserData(vm);

    int key = (int)wrenGetSlotDouble(vm, 1);
    bool is_up = IsKeyUp(key);
    wrenSetSlotBool(vm, 0, is_up == true);
}

static void events_input_iskeypressed(WrenVM *vm)
{
    // Environment_t *environment = (Environment_t *)wrenGetUserData(vm);

    int key = (int)wrenGetSlotDouble(vm, 1);
    bool is_pressed = IsKeyPressed(key);
    wrenSetSlotBool(vm, 0, is_pressed == true);
}

static void events_input_iskeyreleased(WrenVM *vm)
{
    // Environment_t *environment = (Environment_t *)wrenGetUserData(vm);

    int key = (int)wrenGetSlotDouble(vm, 1);
    bool is_released = IsKeyReleased(key);
    wrenSetSlotBool(vm, 0, is_released == true);
}

static void events_environment_quit(WrenVM *vm)
{
    Environment_t *environment = (Environment_t *)wrenGetUserData(vm);

    environment->should_close = true;
}

const Module_Entry_t _modules[] = {
//  { "module", NULL }
    { "graphics", graphics_wren },
    { "events", events_wren },
    { NULL, NULL }
};
const Class_Entry_t _classes[] = {
//  { "module", "className", NULL, NULL }
    { NULL, NULL, NULL, NULL }
};
const Method_Entry_t _methods[] = {
//  { "module", "className", true, "update()", NULL }
    { "graphics", "Canvas", true, "width", graphics_canvas_width },
    { "graphics", "Canvas", true, "height", graphics_canvas_height },
    { "graphics", "Canvas", true, "palette(_)", graphics_canvas_palette },
    { "graphics", "Canvas", true, "bank(_,_,_,_)", graphics_canvas_bank },
    { "graphics", "Canvas", true, "text(_,_,_,_,_,_,_)", graphics_canvas_text },
    { "graphics", "Canvas", true, "point(_,_,_)", graphics_canvas_point },
    { "graphics", "Canvas", true, "polygon(_,_,_)", graphics_canvas_polygon },
    { "graphics", "Canvas", true, "sprite(_,_,_,_,_,_,_)", graphics_canvas_sprite },
    { "events", "Input", true, "isKeyDown(_)", events_input_iskeydown },
    { "events", "Input", true, "isKeyUp(_)", events_input_iskeyup },
    { "events", "Input", true, "isKeyPressed(_)", events_input_iskeypressed },
    { "events", "Input", true, "isKeyReleased(_)", events_input_iskeyreleased },
    { "events", "Environment", true, "quit()", events_environment_quit },
    { NULL, NULL, false, NULL, NULL }
};
