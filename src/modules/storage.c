/*
 * MIT License
 * 
 * Copyright (c) 2019-2024 Marco Lizza
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

#include "storage.h"

#include "internal/udt.h"

#include <core/config.h>
#include <systems/interpreter.h>
#include <systems/storage.h>

static int storage_inject_3ssS_0(lua_State *L);
#if !defined(TOFU_STORAGE_AUTO_COLLECT)
static int storage_flush_0_0(lua_State *L);
#endif  /* TOFU_STORAGE_AUTO_COLLECT */
//static int storage_exists_1s_1b(lua_State *L);

int storage_loader(lua_State *L)
{
    return udt_newmodule(L,
        (const struct luaL_Reg[]){
            // -- operations --
            { "inject", storage_inject_3ssS_0 },
#if !defined(TOFU_STORAGE_AUTO_COLLECT)
            { "flush", storage_flush_0_0 },
#endif  /* TOFU_STORAGE_AUTO_COLLECT */
            { NULL, NULL }
        },
        (const luaX_Const[]){
            { NULL, LUA_CT_NIL, { 0 } }
        });
}

static int storage_inject_3ssS_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
        LUAX_SIGNATURE_OPTIONAL(LUA_TSTRING)
    LUAX_SIGNATURE_END
    const char *name = LUAX_STRING(L, 1);
    luaX_String data = LUAX_LSTRING(L, 2);
    const char *mode = LUAX_OPTIONAL_STRING(L, 3, "base64");

    Storage_t *storage = (Storage_t *)udt_get_userdata(L, USERDATA_STORAGE);

    bool injected;
    switch (mode[0]) {
        case 'a': {
            injected = Storage_inject_ascii85(storage, name, data.data, data.size);
            break;
        }
        case 'b': {
            injected = Storage_inject_base64(storage, name, data.data, data.size);
            break;
        }
        case 'r': {
            injected = Storage_inject_raw(storage, name, data.data, data.size);
            break;
        }
        default: {
            injected = false;
            break;
        }
    }

    if (!injected) {
        return luaL_error(L, "can't inject data `%.32s` as `%s`", data, name);
    }

    return 0;
}

#if !defined(TOFU_STORAGE_AUTO_COLLECT)
static int storage_flush_0_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
    LUAX_SIGNATURE_END

    Storage_t *storage = (Storage_t *)udt_get_userdata(L, USERDATA_STORAGE);

    Storage_flush(storage);

    return 0;
}
#endif  /* TOFU_STORAGE_AUTO_COLLECT */
