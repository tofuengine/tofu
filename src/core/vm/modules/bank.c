/*
 * MIT License
 * 
 * Copyright (c) 2019-2020 Marco Lizza
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
 */

#include "bank.h"

#include <config.h>
#include <core/io/display.h>
#include <libs/fs/fsaux.h>
#include <libs/log.h>
#include <libs/stb.h>

#include "callbacks.h"
#include "structs.h"
#include "udt.h"

#include <math.h>

#define LOG_CONTEXT "bank"
#define META_TABLE  "Tofu_Graphics_Bank_mt"

static int bank_new(lua_State *L);
static int bank_gc(lua_State *L);
static int bank_size(lua_State *L);
static int bank_canvas(lua_State *L);
static int bank_blit(lua_State *L);

static const struct luaL_Reg _bank_functions[] = {
    { "new", bank_new },
    {"__gc", bank_gc },
    { "size", bank_size },
    { "canvas", bank_canvas },
    { "blit", bank_blit },
    { NULL, NULL }
};

int bank_loader(lua_State *L)
{
    int nup = luaX_pushupvalues(L);
    return luaX_newmodule(L, NULL, _bank_functions, NULL, nup, META_TABLE);
}

static GL_Rectangle_t *_load_cells(const File_System_t *file_system, const char *file, size_t *count)
{
    File_System_Mount_t *mount = FS_locate(file_system, file);
    if (!mount) {
        return NULL;
    }

    File_System_Handle_t *handle = FS_open(mount, file);
    if (!handle) {
        return NULL;
    }

    uint32_t entries;
    size_t bytes_requested = sizeof(uint32_t);
    size_t bytes_read = FS_read(handle, &entries, bytes_requested);
    if (bytes_read != bytes_requested) {
        FS_close(handle);
        return NULL;
    }

    GL_Rectangle_t *cells = malloc(sizeof(GL_Rectangle_t) * entries);
    if (!cells) {
        FS_close(handle);
        return NULL;
    }

    for (uint32_t i = 0; i < entries; ++i) {
        Rectangle_u32_t rectangle = (Rectangle_u32_t){ 0 };
        bytes_requested = sizeof(Rectangle_u32_t);
        bytes_read = FS_read(handle, &rectangle, bytes_requested);
        if (bytes_read != bytes_requested) {
            free(cells);
            FS_close(handle);
            return NULL;
        }

        cells[i] = (GL_Rectangle_t){
                .x = rectangle.x,
                .y = rectangle.y,
                .width = rectangle.width,
                .height = rectangle.height
            };
    }

    FS_close(handle);

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "#%d cells loaded from file `%s`", entries, file);

    *count = entries;
    return cells;
}

static int bank_new2(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING, LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
    LUAX_SIGNATURE_END
    int type = lua_type(L, 1);
    const char *cells_file = LUAX_STRING(L, 2);

    const File_System_t *file_system = (const File_System_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_FILE_SYSTEM));
    const Display_t *display = (const Display_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_DISPLAY));

    size_t count;
    GL_Rectangle_t *cells = _load_cells(file_system, cells_file, &count); // TODO: implement `Sheet` in pure Lua?
    if (!cells) {
        return luaL_error(L, "can't load file `%s`", cells_file);
    }

    GL_Sheet_t *sheet;
    if (type == LUA_TSTRING) {
        const char *image_file = LUAX_STRING(L, 1);

        File_System_Resource_t *image = FSX_load(file_system, image_file, FILE_SYSTEM_RESOURCE_IMAGE);
        if (!image) {
            return luaL_error(L, "can't load file `%s`", image_file);
        }
        sheet = GL_sheet_decode(FSX_IWIDTH(image), FSX_IHEIGHT(image), FSX_IPIXELS(image), cells, count, surface_callback_palette, (void *)&display->palette);
        FSX_release(image);
        free(cells);
        if (!sheet) {
            return luaL_error(L, "can't decode file `%s`", image_file);
        }
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "sheet %p decoded from file `%s`", sheet, image_file);
    } else
    if (type == LUA_TUSERDATA) {
        const Canvas_Object_t *canvas = (const Canvas_Object_t *)LUAX_USERDATA(L, 1);

        sheet = GL_sheet_attach(canvas->context->surface, cells, count);
        free(cells);
        if (!sheet) {
            return luaL_error(L, "can't attach sheet");
        }
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "sheet %p attached to canvas %p", sheet, canvas);
    } else {
        free(cells);
        return luaL_error(L, "invalid argument");
    }

    Bank_Object_t *self = (Bank_Object_t *)lua_newuserdatauv(L, sizeof(Bank_Object_t), 1);
    *self = (Bank_Object_t){
            .context = display->context,
            .context_reference = LUAX_REFERENCE_NIL,
            .sheet = sheet,
            .sheet_reference = type == LUA_TUSERDATA ? luaX_ref(L, 1) : LUAX_REFERENCE_NIL
        };
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "bank %p allocated w/ sheet %p for default context", self, sheet);

    luaL_setmetatable(L, META_TABLE);

    return 1;
}

static int bank_new3(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING, LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    int type = lua_type(L, 1);
    size_t cell_width = (size_t)LUAX_INTEGER(L, 2);
    size_t cell_height = (size_t)LUAX_INTEGER(L, 3);

    const File_System_t *file_system = (const File_System_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_FILE_SYSTEM));
    const Display_t *display = (const Display_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_DISPLAY));

    GL_Sheet_t *sheet;
    if (type == LUA_TSTRING) {
        const char *file = LUAX_STRING(L, 1);

        File_System_Resource_t *image = FSX_load(file_system, file, FILE_SYSTEM_RESOURCE_IMAGE);
        if (!image) {
            return luaL_error(L, "can't load file `%s`", file);
        }
        sheet = GL_sheet_decode_rect(FSX_IWIDTH(image), FSX_IHEIGHT(image), FSX_IPIXELS(image), cell_width, cell_height, surface_callback_palette, (void *)&display->palette);
        FSX_release(image);
        if (!sheet) {
            return luaL_error(L, "can't decode file `%s`", file);
        }
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "sheet %p decoded from file `%s`", sheet, file);
    } else
    if (type == LUA_TUSERDATA) {
        const Canvas_Object_t *canvas = (const Canvas_Object_t *)LUAX_USERDATA(L, 1);

        sheet = GL_sheet_attach_rect(canvas->context->surface, cell_width, cell_height);
        if (!sheet) {
            return luaL_error(L, "can't attach sheet");
        }
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "sheet %p attached to canvas %p", sheet, canvas);
    } else {
        return luaL_error(L, "invalid argument");
    }

    Bank_Object_t *self = (Bank_Object_t *)lua_newuserdatauv(L, sizeof(Bank_Object_t), 1);
    *self = (Bank_Object_t){
            .context = display->context,
            .context_reference = LUAX_REFERENCE_NIL,
            .sheet = sheet,
            .sheet_reference = type == LUA_TUSERDATA ? luaX_ref(L, 1) : LUAX_REFERENCE_NIL
        };
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "bank %p allocated w/ sheet %p for default context", self, sheet);

    luaL_setmetatable(L, META_TABLE);

    return 1;
}

static int bank_new(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(2, bank_new2)
        LUAX_OVERLOAD_ARITY(3, bank_new3)
    LUAX_OVERLOAD_END
}

static int bank_gc(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Bank_Object_t *self = (Bank_Object_t *)LUAX_USERDATA(L, 1);

    if (self->sheet_reference != LUAX_REFERENCE_NIL) {
        luaX_unref(L, self->sheet_reference);
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "sheet reference #%d released", self->sheet_reference);
        GL_sheet_detach(self->sheet);
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "sheet %p detached", self->sheet);
    } else {
        GL_sheet_destroy(self->sheet);
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "sheet %p destroyed", self->sheet);
    }

    if (self->context_reference != LUAX_REFERENCE_NIL) {
        luaX_unref(L, self->context_reference);
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "context reference #%d released", self->context_reference);
    }

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "bank %p finalized", self);

    return 0;
}

static int bank_size(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Bank_Object_t *self = (Bank_Object_t *)LUAX_USERDATA(L, 1);
    int cell_id = LUAX_INTEGER(L, 2);
    float scale_x = LUAX_OPTIONAL_NUMBER(L, 3, 1.0f);
    float scale_y = LUAX_OPTIONAL_NUMBER(L, 4, scale_x);

    const GL_Sheet_t *sheet = self->sheet;
    const GL_Rectangle_t *cell = cell_id == -1 ? sheet->cells : &sheet->cells[cell_id]; // If `-1` pick the first one.
    lua_pushinteger(L, (int)(cell->width * fabsf(scale_x)));
    lua_pushinteger(L, (int)(cell->height * fabsf(scale_y)));

    return 2;
}

static int bank_canvas(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_OPTIONAL(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Bank_Object_t *self = (Bank_Object_t *)LUAX_USERDATA(L, 1);
    const Canvas_Object_t *canvas = (Canvas_Object_t *)LUAX_OPTIONAL_USERDATA(L, 2, NULL);

    const Display_t *display = (const Display_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_DISPLAY));

    if (self->context_reference != LUAX_REFERENCE_NIL) {
        luaX_unref(L, self->context_reference);
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "context reference #%d released", self->context_reference);
    }

    if (canvas) {
        self->context = canvas->context;
        self->context_reference = luaX_ref(L, 2);
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "context %p attached w/ reference #%d", self->context, self->context_reference);
    } else {
        self->context = display->context;
        self->context_reference = LUAX_REFERENCE_NIL;
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "default context attached");
    }

    return 0;
}

static int bank_blit4(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Bank_Object_t *self = (Bank_Object_t *)LUAX_USERDATA(L, 1);
    int cell_id = LUAX_INTEGER(L, 2); // FIXME: make cell-id a `size_t' or a generic uint?
    int x = LUAX_INTEGER(L, 3);
    int y = LUAX_INTEGER(L, 4);

    const GL_Context_t *context = self->context;
    const GL_Sheet_t *sheet = self->sheet;
    GL_context_blit(context, sheet->atlas, sheet->cells[cell_id], (GL_Point_t){ .x = x, .y = y });

    return 0;
}

static int bank_blit5(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Bank_Object_t *self = (Bank_Object_t *)LUAX_USERDATA(L, 1);
    int cell_id = LUAX_INTEGER(L, 2);
    int x = LUAX_INTEGER(L, 3);
    int y = LUAX_INTEGER(L, 4);
    float scale = LUAX_NUMBER(L, 5);

    const GL_Context_t *context = self->context;
    const GL_Sheet_t *sheet = self->sheet;
    GL_context_blit_s(context, sheet->atlas, sheet->cells[cell_id], (GL_Point_t){ .x = x, .y = y }, scale, scale);

    return 0;
}

static int bank_blit6_7_8_9(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Bank_Object_t *self = (Bank_Object_t *)LUAX_USERDATA(L, 1);
    int cell_id = LUAX_INTEGER(L, 2);
    int x = LUAX_INTEGER(L, 3);
    int y = LUAX_INTEGER(L, 4);
    int rotation = LUAX_INTEGER(L, 5);
    float scale_x = LUAX_NUMBER(L, 6);
    float scale_y = LUAX_OPTIONAL_NUMBER(L, 7, scale_x);
    float anchor_x = LUAX_OPTIONAL_NUMBER(L, 8, 0.5f);
    float anchor_y = LUAX_OPTIONAL_NUMBER(L, 9, anchor_x);

    const GL_Context_t *context = self->context;
    const GL_Sheet_t *sheet = self->sheet;
    GL_context_blit_sr(context, sheet->atlas, sheet->cells[cell_id], (GL_Point_t){ .x = x, .y = y }, scale_x, scale_y, rotation, anchor_x, anchor_y);

    return 0;
}

static int bank_blit(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(4, bank_blit4)
        LUAX_OVERLOAD_ARITY(5, bank_blit5)
        LUAX_OVERLOAD_ARITY(6, bank_blit6_7_8_9)
        LUAX_OVERLOAD_ARITY(7, bank_blit6_7_8_9)
        LUAX_OVERLOAD_ARITY(8, bank_blit6_7_8_9)
        LUAX_OVERLOAD_ARITY(9, bank_blit6_7_8_9)
    LUAX_OVERLOAD_END
}
