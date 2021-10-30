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

#include "map.h"

#include <string.h>

const Map_Entry_t *map_find_key(lua_State *L, const char *key, const Map_Entry_t *table, size_t size)
{
    for (size_t i = 0; i < size; ++i) {
        const Map_Entry_t *entry = &table[i];
        if (!entry->key) {
            continue;
        }
        if (strcasecmp(entry->key, key) == 0) {
            return entry;
        }
    }

    luaL_error(L, "unknown value for key `%s`", key);
    return NULL;
}

const Map_Entry_t *map_find_value(lua_State *L, Map_Entry_Value_t value, const Map_Entry_t *table, size_t size)
{
    for (size_t i = 0; i < size; ++i) {
        const Map_Entry_t *entry = &table[i];
        if ((Map_Entry_Value_t)entry->value == value) {
            return entry;
        }
    }

    luaL_error(L, "unknown key for value %d", value);
    return NULL;
}