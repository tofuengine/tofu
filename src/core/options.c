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

#include "options.h"

#include <core/platform.h>

#include <stdbool.h>
#include <string.h>

static bool _parse_argument(const char *string, const char *prefix, const char **ptr)
{
    size_t length = strlen(prefix);
    if (strncmp(string, prefix, length) == 0) {
        *ptr = string + length;
        return true;
    }
    return false;
}

options_t options_parse_command_line(int argc, const char *argv[])
{
    options_t options = (options_t) {
        .path = PLATFORM_PATH_CURRENT_SZ
    };

    for (int i = 1; i < argc; ++i) { // Skip executable name, i.e. argument #0.
        if (_parse_argument(argv[i], "--path=", &options.path)) {
            break;
        }
    }

    return options;
}
