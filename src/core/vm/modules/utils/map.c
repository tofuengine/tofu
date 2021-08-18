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

#include <stdlib.h>
#include <string.h>

static int _entry_compare(const void *lhs, const void *rhs)
{
    const Map_Entry_t *l = (const Map_Entry_t *)lhs;
    const Map_Entry_t *r = (const Map_Entry_t *)rhs;
    if (!l->key) {
        return 1;
    } else 
    if (!r->key) {
        return -1;
    } else {
        return strcasecmp(l->key, r->key);
    }
}

const Map_Entry_t *map_find(lua_State *L, const char *id, const Map_Entry_t *table, size_t size)
{
    const Map_Entry_t key = { .key = id };
    const Map_Entry_t *entry = bsearch((const void *)&key, table, size, sizeof(Map_Entry_t), _entry_compare);
    if (!entry) {
        luaL_error(L, "unknown value for key `%s`", id);
        return NULL;
    }
    return entry;
}