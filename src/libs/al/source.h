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

#ifndef __AL_SOURCE_H__
#define __AL_SOURCE_H__

#include <stddef.h>

// TODO: implement both from memory and from file.
typedef size_t (*AL_Source_Read_Callback_t)(void *user_data, void *data, size_t data_size);
typedef void (*AL_Source_Seek_Callback_t)(void *user_data, long position, int whence);

typedef enum _AL_Source_States_t {
    AL_SOURCE_STATE_STOPPED,
    AL_SOURCE_STATE_PLAYING,
    AL_SOURCE_STATE_COMPLETED,
    AL_Source_States_t_CountOf
} AL_Source_States_t;

typedef struct _AL_Source_t {
    bool looped;
    float delay; // ???
    float gain;
    float pan;
    float speed;

    float time; // ???
    AL_Source_States_t state;
    float mix[2];
} AL_Source_t;

extern AL_Source_t *AL_source_create(AL_Source_Read_Callback_t reader, AL_Source_Seek_Callback_t seeker, void *user_data);
extern void AL_source_destroy(AL_Source_t *source);

extern void AL_source_looped(AL_Source_t *source, bool looped);
extern void AL_source_delay(AL_Source_t *source, bool delay);
extern void AL_source_gain(AL_Source_t *source, bool gain);
extern void AL_source_pan(AL_Source_t *source, bool pan);
extern void AL_source_speed(AL_Source_t *source, bool speed);

extern void AL_source_pause(AL_Source_t *source);
extern void AL_source_resume(AL_Source_t *source);
extern void AL_source_stop(AL_Source_t *source);

extern void AL_source_update(AL_Source_t *source, float delta_time);

#endif  /* __AL_SOURCE_H__ */