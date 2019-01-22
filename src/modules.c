#include "modules.h"

#include <raylib/raylib.h>
#include <string.h>

#include "environment.h"
#include "log.h"

#include "modules/graphics_wren.inc"

#define SLOT_AUX        0

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

const Module_Entry_t _modules[] = {
//  { "module", NULL }
    { "graphics", graphics_wren },
    { NULL, NULL }
};
const Class_Entry_t _classes[] = {
//  { "module", "className", NULL, NULL }
    { NULL, NULL, NULL, NULL }
};
const Method_Entry_t _methods[] = {
//  { "module", "className", true, "update()", NULL }
    { "graphics", "Canvas", true, "point(_,_,_)", graphics_canvas_point },
    { "graphics", "Canvas", true, "palette(_)", graphics_canvas_palette },
    { "graphics", "Canvas", true, "bank(_,_,_,_)", graphics_canvas_bank },
    { "graphics", "Canvas", true, "sprite(_,_,_,_,_,_,_)", graphics_canvas_sprite },
    { NULL, NULL, false, NULL, NULL }
};
