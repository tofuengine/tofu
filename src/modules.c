#include "modules.h"

#include <raylib/raylib.h>

#define GRAPHICS_MODULE \
    "foreign class Canvas {\n" \
    "\n" \
    "    foreign static point(x, y, color)\n" \
    "\n" \
    "}\n"

static void graphics_canvas_point(WrenVM* vm)
{
    int x = (int)wrenGetSlotDouble(vm, 1); 
    int y = (int)wrenGetSlotDouble(vm, 2); 
    int color = (int)wrenGetSlotDouble(vm, 3); 

    DrawPixel(x, y, (Color){ color, color, color, 255 });
}

const Module_Entry_t _modules[] = {
//  { "module", NULL }
    { "graphics", GRAPHICS_MODULE },
    { NULL, NULL }
};
const Class_Entry_t _classes[] = {
//  { "module", "className", NULL, NULL }
    { NULL, NULL, NULL, NULL }
};
const Method_Entry_t _methods[] = {
//  { "module", "className", true, "update()", NULL }
    { "graphics", "Canvas", true, "point(_,_,_)", graphics_canvas_point },
    { NULL, NULL, false, NULL, NULL }
};
