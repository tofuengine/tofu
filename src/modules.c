#include "modules.h"

#include <raylib/raylib.h>

#include "modules/graphics_wren.inc"

static void graphics_canvas_point(WrenVM* vm)
{
    int x = (int)wrenGetSlotDouble(vm, 1); 
    int y = (int)wrenGetSlotDouble(vm, 2); 
    int color = (int)wrenGetSlotDouble(vm, 3); 

    DrawPixel(x, y, (Color){ color, color, color, 255 });
}

static void graphics_canvas_palette(WrenVM* vm)
{
    int count = wrenGetListCount(vm, 0);
    for (int i = 0; i < count; ++i) {
        wrenGetListElement(vm, 0, i, 1);
        const char *hex = wrenGetSlotString(vm, 1);
        // TODO: pack the colors and send to the shader.
    }
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
    { NULL, NULL, false, NULL, NULL }
};
