/*
 * MIT License
 * 
 * Copyright (c) 2019-2023 Marco Lizza
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

#ifndef TOFU_LIBS_PROFILE_H
#define TOFU_LIBS_PROFILE_H

#include "log.h"

typedef struct Profile_s {
    double marker;
} Profile_t;

#define PROFILE_BEGIN(context, tag) \
    do { \
        const char *_context = (context); \
        const char *_tag = (tag); \
        Profile_t _profile; \
        profile_init(&_profile);
#define PROFILE_END \
        float _elapsed = profile_elapsed(&_profile); \
        LOG_D(_context, "`%s` took %.3fs", _tag, _elapsed); \
    } while (0)

extern void profile_init(Profile_t *profile);
extern float profile_elapsed(const Profile_t *profile);

#endif  /* TOFU_LIBS_PROFILE_H */
