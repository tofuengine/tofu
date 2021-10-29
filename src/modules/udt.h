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

#include <chipmunk/chipmunk.h>

#include <libs/easing.h>
#include <libs/luax.h>
#include <libs/noise.h>
#include <libs/wave.h>
#include <libs/fs/fs.h>
#include <libs/gl/gl.h>
#include <libs/sl/sl.h>
#include <systems/display.h>

typedef enum UserData_e { // TODO: move to a separate file.
    USERDATA_STORAGE = 1,
    USERDATA_DISPLAY,
    USERDATA_INPUT,
    USERDATA_AUDIO,
    USERDATA_ENVIRONMENT,
    USERDATA_PHYSICS,
    USERDATA_INTERPRETER,
    UserData_t_CountOf
} UserData_t;

typedef enum Object_Types_e {
    // Graphics
    OBJECT_TYPE_BANK,
    OBJECT_TYPE_BATCH,
    OBJECT_TYPE_BODY,
    OBJECT_TYPE_CANVAS,
    OBJECT_TYPE_FONT,
    OBJECT_TYPE_GRID,
    OBJECT_TYPE_PALETTE,
    OBJECT_TYPE_PROGRAM,
    OBJECT_TYPE_XFORM,
    // Sound
    OBJECT_TYPE_SOURCE,
    // Math
    OBJECT_TYPE_NOISE,
    OBJECT_TYPE_TWEENER,
    OBJECT_TYPE_WAVE
} Object_Types_t;

typedef struct Canvas_Object_s {
    GL_Surface_t *surface;
    bool allocated;
    struct {
        GL_Pixel_t background, foreground;
    } color;
} Canvas_Object_t;

typedef struct Bank_Object_s {
    struct {
        const Canvas_Object_t *instance;
        luaX_Reference reference;
    } atlas;
    GL_Sheet_t *sheet;
} Bank_Object_t;

typedef struct Font_Object_s {
    struct {
        const Canvas_Object_t *instance;
        luaX_Reference reference;
    } atlas;
    GL_Sheet_t *sheet;
    GL_Cell_t glyphs[256];
} Font_Object_t;

typedef struct Batch_Object_s {
    struct {
        const Bank_Object_t *instance;
        luaX_Reference reference;
    } bank;
    GL_Batch_t *batch;
} Batch_Object_t;

typedef struct XForm_Object_s {
    GL_XForm_t *xform;
} XForm_Object_t;

typedef struct Palette_Object_s {
    GL_Palette_t *palette;
} Palette_Object_t;

typedef struct Program_Object_s {
    GL_Program_t *program;
} Program_Object_t;

#ifdef __GRID_INTEGER_CELL__
typedef int Cell_t;
#else
typedef float Cell_t;
#endif

typedef struct Grid_Object_s {
    size_t width, height;
    Cell_t *data;
    size_t data_size;
} Grid_Object_t;

typedef struct Source_Object_s {
    FS_Handle_t *handle;
    SL_Source_t *source;
} Source_Object_t;

typedef enum Body_Kinds_e {
    BODY_KIND_SHAPELESS,
    BODY_KIND_BOX,
    BODY_KIND_CIRCLE,
    Body_Kinds_t_CountOf
} Body_Kinds_t;

typedef struct Body_Object_s {
    cpBody *body;
    cpShape *shape;
    Body_Kinds_t kind;
    union {
        struct {
            cpFloat width, height, radius;
        } box;
        struct {
            cpFloat radius;
            cpVect offset;
        } circle;
    } size;
//    cpFloat *momentum;
} Body_Object_t;

typedef enum Easing_Types_e {
    EASING_TYPE_LINEAR,
    EASING_TYPE_QUADRATIC_IN,
    EASING_TYPE_QUADRATIC_OUT,
    EASING_TYPE_QUADRATIC_IN_OUT,
    EASING_TYPE_CUBIC_IN,
    EASING_TYPE_CUBIC_OUT,
    EASING_TYPE_CUBIC_IN_OUT,
    EASING_TYPE_QUARTIC_IN,
    EASING_TYPE_QUARTIC_OUT,
    EASING_TYPE_QUARTIC_IN_OUT,
    EASING_TYPE_QUINTIC_IN,
    EASING_TYPE_QUINTIC_OUT,
    EASING_TYPE_QUINTIC_IN_OUT,
    EASING_TYPE_SINE_IN,
    EASING_TYPE_SINE_OUT,
    EASING_TYPE_SINE_IN_OUT,
    EASING_TYPE_CIRCULAR_IN,
    EASING_TYPE_CIRCULAR_OUT,
    EASING_TYPE_CIRCULAR_IN_OUT,
    EASING_TYPE_EXPONENTIAL_IN,
    EASING_TYPE_EXPONENTIAL_OUT,
    EASING_TYPE_EXPONENTIAL_IN_OUT,
    EASING_TYPE_ELASTIC_IN,
    EASING_TYPE_ELASTIC_OUT,
    EASING_TYPE_ELASTIC_IN_OUT,
    EASING_TYPE_BACK_IN,
    EASING_TYPE_BACK_OUT,
    EASING_TYPE_BACK_IN_OUT,
    EASING_TYPE_BOUNCE_OUT,
    EASING_TYPE_BOUNCE_IN,
    EASING_TYPE_BOUNCE_IN_OUT,
    Easing_Types_t_CountOf
} Easing_Types_t;

typedef struct Tweener_Object_s {
    Easing_Types_t easing;
    Easing_Function_t function;
    float duration;
    float from;
    float to;
} Tweener_Object_t;

typedef enum Noise_Types_e {
    NOISE_TYPE_PERLIN,
    NOISE_TYPE_SIMPLEX,
    NOISE_TYPE_CELLULAR,
    Noise_Types_t_CountOf
} Noise_Types_t;

typedef struct Noise_Object_s {
    Noise_Types_t type;
    Noise_Function_t function;
    float seed;
    float frequency;
} Noise_Object_t;

typedef enum Wave_Types_e {
    WAVE_TYPE_SINE,
    WAVE_TYPE_SQUARE,
    WAVE_TYPE_TRIANGLE,
    WAVE_TYPE_SAWTOOTH,
    Wave_Types_t_CountOf
} Wave_Types_t;

typedef struct Wave_Object_s {
    Wave_Types_t form;
    Wave_Function_t function;
    float period;
    float amplitude;
} Wave_Object_t;

#endif  /* __MODULES_UDT_H__ */
