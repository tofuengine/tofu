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

#ifndef __MODULES_UDT_H__
#define __MODULES_UDT_H__

#include <libs/luax.h>
#include <libs/gl/gl.h>

typedef enum _UserData_t { // TODO: move to a suitable space.
    USERDATA_INTERPRETER = 1,
    USERDATA_FILE_SYSTEM,
    USERDATA_ENVIRONMENT,
    USERDATA_DISPLAY,
    USERDATA_INPUT
} UserData_t;

#if 0
// TODO: add type as first field of a `Class_t` type to track proper type and avoid errors.
typedef enum _Classes_t {
    CLASS_CANVAS,
    CLASS_BANK,
    CLASS_FONT,
    CLASS_XFORM
} Classes_t;

typedef struct _Class_t {
    Classes_t type;
} Class_t;
#endif

typedef struct _Canvas_Class_t {
    GL_Context_t *context;
    bool allocated;
} Canvas_Class_t;

typedef struct _Bank_Class_t {
    GL_Context_t *context;
    luaX_Reference context_reference;
    GL_Sheet_t *sheet;
    luaX_Reference sheet_reference;
} Bank_Class_t;

typedef struct _Font_Class_t {
    GL_Context_t *context;
    luaX_Reference context_reference;
    GL_Sheet_t *sheet;
    luaX_Reference sheet_reference;
} Font_Class_t;

typedef struct _XForm_Class_t {
    GL_Context_t *context;
    luaX_Reference context_reference;
    GL_Surface_t *surface;
    luaX_Reference surface_reference;
    GL_XForm_t xform;
} XForm_Class_t;

#ifdef __GRID_INTEGER_CELL__
typedef int Cell_t;
#else
typedef float Cell_t;
#endif

typedef struct _Grid_Class_t {
    size_t width, height;
    Cell_t *data;
    size_t data_size;
} Grid_Class_t;

typedef struct _Vector_Class_t {
    float x, y;
} Vector_Class_t;

#endif  /* __MODULES_UDT_H__ */