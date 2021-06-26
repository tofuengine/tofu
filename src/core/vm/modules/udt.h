/*
 * MIT License
 * 
 * Copyright (c) 2019-2021 Marco Lizza
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

#ifndef __MODULES_UDT_H__
#define __MODULES_UDT_H__

#include <core/io/display.h>
#include <libs/luax.h>
#include <libs/fs/fs.h>
#include <libs/gl/gl.h>
#include <libs/sl/sl.h>

typedef enum _UserData_t { // TODO: move to a separate file.
    USERDATA_STORAGE = 1,
    USERDATA_DISPLAY,
    USERDATA_INPUT,
    USERDATA_AUDIO,
    USERDATA_ENVIRONMENT,
    USERDATA_INTERPRETER,
    UserData_t_CountOf
} UserData_t;

typedef enum _Object_Types_t {
    OBJECT_TYPE_CANVAS,
    OBJECT_TYPE_BANK,
    OBJECT_TYPE_FONT,
    OBJECT_TYPE_BATCH,
    OBJECT_TYPE_XFORM,
    OBJECT_TYPE_PALETTE,
    OBJECT_TYPE_PROGRAM,
    OBJECT_TYPE_GRID,
    OBJECT_TYPE_SOURCE
} Object_Types_t;

LUAX_TYPE_BEGIN(Canvas_Object_t)
    GL_Surface_t *surface;
    bool allocated;
    struct {
        GL_Pixel_t background, foreground;
    } color;
//    struct {
//        const Bank_Object_t *instance;
//        luaX_Reference reference;
//    } bank;
LUAX_TYPE_END

LUAX_TYPE_BEGIN(Bank_Object_t)
    struct {
        const Canvas_Object_t *instance;
        luaX_Reference reference;
    } atlas;
    GL_Sheet_t *sheet;
LUAX_TYPE_END

LUAX_TYPE_BEGIN(Font_Object_t)
    struct {
        const Canvas_Object_t *instance;
        luaX_Reference reference;
    } atlas;
    GL_Sheet_t *sheet;
    GL_Cell_t glyphs[256];
LUAX_TYPE_END

LUAX_TYPE_BEGIN(Batch_Object_t)
    struct {
        const Bank_Object_t *instance;
        luaX_Reference reference;
    } bank;
    GL_Batch_t *batch;
LUAX_TYPE_END

LUAX_TYPE_BEGIN(XForm_Object_t)
    GL_XForm_t *xform;
LUAX_TYPE_END

LUAX_TYPE_BEGIN(Palette_Object_t)
    GL_Palette_t *palette;
LUAX_TYPE_END

LUAX_TYPE_BEGIN(Program_Object_t)
    GL_Program_t *program;
LUAX_TYPE_END

#ifdef __GRID_INTEGER_CELL__
typedef int Cell_t;
#else
typedef float Cell_t;
#endif

LUAX_TYPE_BEGIN(Grid_Object_t)
    size_t width, height;
    Cell_t *data;
    size_t data_size;
LUAX_TYPE_END

LUAX_TYPE_BEGIN(Source_Object_t)
    FS_Handle_t *handle;
    SL_Source_t *source;
LUAX_TYPE_END

#endif  /* __MODULES_UDT_H__ */
