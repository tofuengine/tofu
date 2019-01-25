#include "modules.h"

#include <raylib/raylib.h>
#include <string.h>

#include "environment.h"
#include "log.h"

#include "modules/graphics_wren.inc"
#include "modules/events_wren.inc"

#define UNUSED(x)       (void)(x)

#define SLOT_AUX        0

static void graphics_canvas_width(WrenVM *vm)
{
    Environment_t *environment = (Environment_t *)wrenGetUserData(vm);
    wrenSetSlotDouble(vm, 0, environment->graphics.width);
}

static void graphics_canvas_height(WrenVM *vm)
{
    Environment_t *environment = (Environment_t *)wrenGetUserData(vm);
    wrenSetSlotDouble(vm, 0, environment->graphics.height);
}

static void graphics_canvas_point(WrenVM *vm)
{
    int x = (int)wrenGetSlotDouble(vm, 1); 
    int y = (int)wrenGetSlotDouble(vm, 2); 
    int color = (int)wrenGetSlotDouble(vm, 3); 

    DrawPixel(x, y, (Color){ color, color, color, 255 });
}

static void graphics_canvas_palette(WrenVM *vm)
{
    int count = wrenGetListCount(vm, 1);
    Log_write(LOG_LEVELS_DEBUG, "Canvas.palette() -> %d", count);
    for (int i = 0; i < count; ++i) {
        wrenGetListElement(vm, 1, i, SLOT_AUX); // Use slot #0 as a temporary slot.
        int color = (int)wrenGetSlotDouble(vm, SLOT_AUX);
        // const char *hex = wrenGetSlotString(vm, SLOT_AUX);
        Log_write(LOG_LEVELS_DEBUG, " %d", color);
        // TODO: pack the colors and send to the shader.
    }
}

static void graphics_canvas_bank(WrenVM *vm)
{
    int bank_id = (int)wrenGetSlotDouble(vm, 1);
    const char *file = wrenGetSlotString(vm, 2);
    int cell_width = (int)wrenGetSlotDouble(vm, 3);
    int cell_height = (int)wrenGetSlotDouble(vm, 4);
    Log_write(LOG_LEVELS_DEBUG, "Canvas.bank() -> %d, %s, %d, %d", bank_id, file, cell_width, cell_height);

    Environment_t *environment = (Environment_t *)wrenGetUserData(vm);

    char pathfile[PATH_FILE_MAX] = {};
    strcpy(pathfile, environment->base_path);
    strcat(pathfile, file + 2);

    Texture2D texture = LoadTexture(pathfile);
    Log_write(LOG_LEVELS_DEBUG, "Texture '%s' loaded w/ id #%d", pathfile, texture.id);

    environment->graphics.banks[bank_id] = (Bank_t){
            .atlas = texture,
            .cell_width = cell_width,
            .cell_height = cell_height
        };
}

static void graphics_canvas_sprite(WrenVM *vm)
{
    int bank_id = (int)wrenGetSlotDouble(vm, 1);
    int sprite_id = (int)wrenGetSlotDouble(vm, 2);
    int x = (int)wrenGetSlotDouble(vm, 3);
    int y = (int)wrenGetSlotDouble(vm, 4);
    double rotation = wrenGetSlotDouble(vm, 5);
    double scale_x = wrenGetSlotDouble(vm, 6);
    double scale_y = wrenGetSlotDouble(vm, 7);
//    Log_write(LOG_LEVELS_DEBUG, "Canvas.sprite() -> %d, %d, %d, %d, %.3f, %.3f, %.3f", bank_id, sprite_id, x, y, rotation, scale_x, scale_y);

    Environment_t *environment = (Environment_t *)wrenGetUserData(vm);
    const Bank_t *bank = &environment->graphics.banks[bank_id];

    if (bank->atlas.id == 0) {
        return;
    }

    Rectangle sourceRec = { sprite_id * bank->cell_width, 0.0f, (float)bank->cell_width, (float)bank->cell_height };
    Rectangle destRec = { x, y, (float)bank->cell_width * scale_x, (float)bank->cell_height * scale_y };
    Vector2 origin = { bank->cell_width / 2.0f, bank->cell_height / 2.0f };

    DrawTexturePro(bank->atlas, sourceRec, destRec, origin, (float)rotation, (Color){ 255, 255, 255, 255 });
}

static void graphics_canvas_text(WrenVM *vm) // foreign static text(font_id, text, color, x, y, r, sx, sy)
{
    int font_id = (int)wrenGetSlotDouble(vm, 1);
    const char *text = wrenGetSlotString(vm, 2);
    int x = (int)wrenGetSlotDouble(vm, 3);
    int y = (int)wrenGetSlotDouble(vm, 4);
    int color = (int)wrenGetSlotDouble(vm, 5);
    int size = (int)wrenGetSlotDouble(vm, 6);
    // const char *align = wrenGetSlotString(vm, 7);

    UNUSED(font_id);
    // UNUSED(align);

    DrawText(text, x, y, size, (Color){ color, color, color, 255 });
}

static void events_input_iskeydown(WrenVM *vm)
{
    int key = (int)wrenGetSlotDouble(vm, 1);
    bool is_down = IsKeyDown(key);
    wrenSetSlotBool(vm, 0, is_down == true);
}

static void events_input_iskeypressed(WrenVM *vm)
{
    int key = (int)wrenGetSlotDouble(vm, 1);
    bool is_down = IsKeyPressed(key);
    wrenSetSlotBool(vm, 0, is_down == true);
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
    { "graphics", "Canvas", true, "point(_,_,_)", graphics_canvas_point },
    { "graphics", "Canvas", true, "palette(_)", graphics_canvas_palette },
    { "graphics", "Canvas", true, "bank(_,_,_,_)", graphics_canvas_bank },
    { "graphics", "Canvas", true, "sprite(_,_,_,_,_,_,_)", graphics_canvas_sprite },
    { "graphics", "Canvas", true, "text(_,_,_,_,_,_)", graphics_canvas_text },
    { "events", "Input", true, "isKeyPressed(_)", events_input_iskeypressed },
    { "events", "Input", true, "isKeyDown(_)", events_input_iskeydown },
    { "events", "Environment", true, "quit()", events_environment_quit },
    { NULL, NULL, false, NULL, NULL }
};
