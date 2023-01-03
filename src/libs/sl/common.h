/*
 * MIT License
 *
 * Copyright (c) 2019-2022 Marco Lizza
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

#ifndef TOFU_LIBS_SL_COMMON_H
#define TOFU_LIBS_SL_COMMON_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

// We could use floating point format for simpler and more consistent mixing. Two channels are enough to have some
// panning effects. A sample rate of 48kHz is the optimal choice since it's the internal default for many soundcards
// and converting from lower sample rates is simpler.
#define SL_BYTES_PER_SAMPLE     2
#define SL_SAMPLES_PER_CHANNEL  1
#define SL_CHANNELS_PER_FRAME   2
#define SL_FRAMES_PER_SECOND    22050

#define SL_BYTES_PER_FRAME      (SL_CHANNELS_PER_FRAME * SL_SAMPLES_PER_CHANNEL * SL_BYTES_PER_SAMPLE)

#define SL_GROUPS_AMOUNT        256
#define SL_FIRST_GROUP          0
#define SL_LAST_GROUP           (SL_GROUPS_AMOUNT - 1)
#define SL_DEFAULT_GROUP        SL_FIRST_GROUP
#define SL_ANY_GROUP            (SL_LAST_GROUP + 1)

typedef struct SL_Callbacks_s {
    size_t (*read)(void *user_data, void *buffer, size_t bytes_to_read);
    bool   (*seek)(void *user_data, long offset, int whence);
    long   (*tell)(void *user_data);
    int    (*eof)(void *user_data);
    void *user_data;
} SL_Callbacks_t;

typedef struct SL_Mix_s {
    float left_to_left, left_to_right;
    float right_to_left, right_to_right;
} SL_Mix_t;

typedef struct SL_Group_s {
    SL_Mix_t mix;
    float gain;
} SL_Group_t;

#endif  /* TOFU_LIBS_SL_COMMON_H */
