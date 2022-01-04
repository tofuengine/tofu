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

#include "storage.h"

#include <config.h>
#include <libs/luax.h>
#include <systems/interpreter.h>
#include <systems/storage.h>

#include "udt.h"

static int storage_inject_3ssS_0(lua_State *L);
static int storage_scan_1f_0(lua_State *L);

int storage_loader(lua_State *L)
{
    int nup = luaX_pushupvalues(L);
    return luaX_newmodule(L, (luaX_Script){ 0 },
        (const struct luaL_Reg[]){
            { "inject", storage_inject_3ssS_0 },
            { "scan", storage_scan_1f_0 },
            { NULL, NULL }
        },
        (const luaX_Const[]){
            { NULL, LUA_CT_NIL, { 0 } }
        }, nup, NULL);
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

    Storage_t *storage = (Storage_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_STORAGE));

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

typedef struct Storage_Scan_Closure_s {
    const Storage_t *storage;
    const Interpreter_t *interpreter;
    lua_State *L;
    int index;
} Storage_Scan_Closure_t;

static void _scan_callback(void *user_data, const char *name)
{
    const Storage_Scan_Closure_t *closure = (const Storage_Scan_Closure_t *)user_data;

    lua_pushvalue(closure->L, closure->index); // Copy directly from stack argument, don't need to ref/unref (won't be GC-ed meanwhile)
    lua_pushstring(closure->L, name);
    Interpreter_call(closure->interpreter, 1, 0);
}

static int storage_scan_1f_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TFUNCTION)
    LUAX_SIGNATURE_END
//    luaX_Reference callback = luaX_tofunction(L, 1);

    const Storage_t *storage = (const Storage_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_STORAGE));
    const Interpreter_t *interpreter = (const Interpreter_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_INTERPRETER));

    Storage_scan(storage, _scan_callback, &(Storage_Scan_Closure_t){
            .storage = storage,
            .interpreter = interpreter,
            .L = L,
            .index = 1
        });

    return 0;
}
