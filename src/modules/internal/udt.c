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

#include "udt.h"

#define _LOG_TAG "udt"
#include <libs/log.h>
#include <libs/path.h>

#include <systems/storage.h>

// We tail-add the module name in the upvalues, as it will later used in the loading process both to
// find the (optional) script name and as a metatable name (when a class is created).
void udt_preload_modules(lua_State *L, const void *userdatas[], const luaL_Reg *modules)
{
    int nup = 0;
    for (int i = 0; userdatas[i]; ++i) {
        lua_pushlightuserdata(L, (void *)userdatas[i]); // Discard `const` qualifier.
        nup += 1;
    }

    for (const luaL_Reg *module = modules; module->func; ++module) {
        LOG_D("preloading module `%s`", module->name);
        luaX_pushvalues(L, nup);
        lua_pushstring(L, module->name); // Tail-add the module name (for later usage during the loading process)
        luaX_preload(L, module->name, module->func, nup + 1);
    }

    lua_pop(L, nup); // Free the upvalues from the stack.
}

static const char *_get_module_name(lua_State *L)
{
    return LUAX_STRING(L, lua_upvalueindex(UPVALUE_MODULE_NAME));
}

// FIXME: derive something different from the module as metatable name?
static const char *_get_metatable_name(lua_State *L)
{
    // Any identifier is valid, as long as it won't clash with another metatable in the registry table. The module
    // name (which is a "namespace" in our context) is unique enough.
    return LUAX_STRING(L, lua_upvalueindex(UPVALUE_MODULE_NAME));
}

// This wraps the `LuaX` module creation API by:
//
// 1) retrieving the module-name from the upvalues so that it is automatically carried along the code w/o the need to
//    double-define it somewhere else;
// 2) checking it a `.lua` module script is present, and in case use it to bootstrap the module initialization to
//    support mixed Lua-and-C module code;
// 3) pass the module name as meta-table identifier, as this is required when a object-constructor is implemented in
//    C.
//
// Technically the step #3 is required only for instantiable (i.e. non-static) classes. However, it makes not harm to
// have it uniformly set for every (module) table.
int udt_newmodule(lua_State *L, const luaL_Reg *f, const luaX_Const *c)
{
    const char *module_name = _get_module_name(L);
    LOG_D("loading module `%s`", module_name);

    char name[PLATFORM_PATH_MAX] = { 0 };
    const char *file = path_lua_to_fs(name, module_name);

    Storage_t *storage = (Storage_t *)udt_get_userdata(L, USERDATA_STORAGE);
    Storage_Resource_t *script = Storage_load(storage, file, STORAGE_RESOURCE_STRING);
    LOG_IF_D(script, "loading script `%s`", file);

    int nup = luaX_pushupvalues(L);
    return luaX_newmodule(L,
        (luaX_Script){
            .data = script ? SR_SCHARS(script) : NULL,
            .size = script ? SR_SLENTGH(script) : 0,
            .name = name
        }, f, c, nup, _get_metatable_name(L));
    // Note: the object creation could be potentially faster if we store as an upvalue the metatable itself,
    //       which will save us a "name to metatable" lookup during the object creation.
}

// This one pairs with `udt_newmodule()`, in the sense that it's merely a wrapping function that passes along to the
// `luaX_newobject()` function the metatable name fetched as an upvalue.
void *udt_newobject(lua_State *L, size_t size, void *state, int type)
{
    return luaX_newobject(L, size, state, type, _get_metatable_name(L));
}

void *udt_get_userdata(lua_State *L, UserData_t id)
{
    return LUAX_USERDATA(L, lua_upvalueindex(id));
}
