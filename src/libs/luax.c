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

#include "luax.h"

#include <string.h>

/*
http://webcache.googleusercontent.com/search?q=cache:RLoR9dkMeowJ:howtomakeanrpg.com/a/classes-in-lua.html+&cd=4&hl=en&ct=clnk&gl=it
https://hisham.hm/2014/01/02/how-to-write-lua-modules-in-a-post-module-world/
https://www.oreilly.com/library/view/creating-solid-apis/9781491986301/ch01.html
file:///C:/Users/mlizza/Downloads/[Roberto_Ierusalimschy]_Programming_in_Lua(z-lib.org).pdf (page 269)
https://nachtimwald.com/2014/07/12/wrapping-a-c-library-in-lua/
https://www.lua.org/pil/28.5.html
https://stackoverflow.com/questions/16713837/hand-over-global-custom-data-to-lua-implemented-functions
https://stackoverflow.com/questions/29449296/extending-lua-check-number-of-parameters-passed-to-a-function
https://stackoverflow.com/questions/32673835/how-do-i-create-a-lua-module-inside-a-lua-module-in-c
*/

void *luaX_newtype(lua_State *L, size_t size, void *object, int type) // FIXME: rename to `newobject` and `isobject`
{
    void *obj = lua_newuserdatauv(L, sizeof(luaX_Object) + size, 1);
    ((luaX_Object *)obj)->type = type;
    void *self = (uint8_t *)obj + sizeof(luaX_Object);
    memcpy(self, object, size);
    return self;
}

int luaX_istype(lua_State *L, int idx, int type)
{
    luaX_Object *obj = (luaX_Object *)lua_touserdata(L, idx);
    if (!obj) {
        return 0;
    }
    return obj->type == type;
}

void *luaX_totype(lua_State *L, int idx, int type)
{
    luaX_Object *obj = (luaX_Object *)lua_touserdata(L, idx);
    if (!obj) {
        return 0;
    }
    return obj->type == type ? obj + 1: NULL;
}

void luaX_stackdump(lua_State *L, const char* func, int line)
{
    int top = lua_gettop(L);
    printf("----------[ STACK DUMP (%s:%d) top=%d ]----------\n", func, line, top);
    for (int i = 0; i < top; ++i) {
        int positive = top - i;
        int negative = -(i + 1);
        int type = lua_type(L, positive);
#if 0
        int typeN = lua_type(L, negative);
        if (type != typeN) {
            printf("  %d/%d: type mismatch %d != %d\n", positive, negative, type, typeN);
            continue;
        }
#endif
        const char* type_name = lua_typename(L, type);
        printf("  %d/%d: type=%s", positive, negative, type_name);
        switch (type) {
            case LUA_TBOOLEAN:
                printf("\t%s", lua_toboolean(L, i) ? "true" : "false");
                break;
            case LUA_TLIGHTUSERDATA:
                printf("\t%p", lua_topointer(L, positive));
                break;
            case LUA_TNUMBER:
                printf("\t%f", lua_tonumber(L, positive));
                break;
            case LUA_TSTRING:
                printf("\t%s", lua_tostring(L, positive));
                break;
            case LUA_TTABLE:
                printf("\t%p", lua_topointer(L, positive));
                break;
            case LUA_TFUNCTION:
                if (lua_iscfunction(L, positive)) {
                    union {
                        lua_CFunction f;
                        const void *p;
                    } u = { .f = lua_tocfunction(L, positive) }; // Trick the compiler to print function address.
                    printf("\t%p", u.p);
                } else {
                    printf("\t%p", lua_topointer(L, positive));
                }
                break;
            case LUA_TUSERDATA:
                printf("\t%p", lua_topointer(L, positive));
                break;
            case LUA_TTHREAD:
                printf("\t%p", lua_topointer(L, positive));
                break;
            default:
                printf("\t<skipped>");
                break;
        }
        printf("\n");
    }
}

void luaX_overridesearchers(lua_State *L, lua_CFunction searcher, int nup)
{
    lua_getglobal(L, "package"); // Access the `package.searchers` table.
    lua_getfield(L, -1, "searchers");
    lua_insert(L, -(nup + 2)); // Move the `searchers` table above the upvalues.
    lua_insert(L, -(nup + 2)); // Move the `package` table above the upvalues.
    lua_pushcclosure(L, searcher, nup);
    lua_rawseti(L, -2, 2); // Override the 2nd searcher (keep the "preloaded" helper).

    int n = lua_rawlen(L, -1);
    for (int i = 3; i <= n; ++i) { // Discard the others (two) searchers.
        lua_pushnil(L);
        lua_rawseti(L, -2, i);
    }

    lua_pop(L, 2); // Pop the `package` and `searchers` table.
}

int luaX_insisttable(lua_State *L, const char *name)
{
    lua_getglobal(L, name);

    if (!lua_istable(L, -1)) {
        lua_pop(L, 1); // Pop the non-table.
        lua_newtable(L);
        lua_pushvalue(L, -1);
        lua_setglobal(L, name);
    }

    return 1;
}

// Both `f` and `c` can't be `NULL`, but need to be "empty" arrays (which is easy, thanks to compound-literals)
int luaX_newmodule(lua_State *L, luaX_Script script, const luaL_Reg *f, const luaX_Const *c, int nup, const char *name)
{
    if (script.buffer && script.size > 0) {
        luaL_loadbuffer(L, script.buffer, script.size, script.name);
        lua_pcall(L, 0, LUA_MULTRET, 0); // Just the export table is returned.
        if (name) {
            lua_pushstring(L, name);
            lua_setfield(L, -2, "__name");  /* metatable.__name = tname */
            lua_pushvalue(L, -1);
            lua_setfield(L, LUA_REGISTRYINDEX, name);  /* registry.name = metatable */
        }
    } else
    if (name) {
        luaL_newmetatable(L, name); // create metatable
    } else {
        lua_newtable(L); // create nameless metatable, in case of a non-class.
    }

    // Duplicate the metatable, since it will be popped by the 'lua_setfield()' call.
    // This is equivalent to the following in lua:
    // metatable = {}
    // metatable.__index = metatable
    if (name) {
        lua_pushvalue(L, -1); // Possibly redundant, if already done in the script.
        lua_setfield(L, -2, "__index");  /* metatable.__index = metatable */
    }

    lua_insert(L, -(nup + 1)); // Move the table above the upvalues (to permit them to be consumed while preservig the table).
    luaL_setfuncs(L, f, nup); // Register the function into the table at the top of the stack, i.e. create the methods.

    for (; c->name; c++) {
        switch (c->type) {
            case LUA_CT_NIL: { lua_pushnil(L); } break;
            case LUA_CT_BOOLEAN: { lua_pushboolean(L, c->value.b); } break;
            case LUA_CT_INTEGER: { lua_pushinteger(L, (lua_Integer)c->value.i); } break;
            case LUA_CT_NUMBER: { lua_pushnumber(L, (lua_Number)c->value.n); } break;
            case LUA_CT_STRING: { lua_pushstring(L, c->value.sz); } break;
        }
        lua_setfield(L, -2, c->name);
    }

    // Upvalues have already been consumed by `luaL_setfuncs()`. No need to clear the stack.

    return 1;
}

void luaX_openlibs(lua_State *L)
{
    static const luaL_Reg libraries[] = {
        { LUA_GNAME, luaopen_base },
        { LUA_LOADLIBNAME, luaopen_package },
        { LUA_COLIBNAME, luaopen_coroutine },
        { LUA_TABLIBNAME, luaopen_table },
#ifdef __INCLUDE_SYSTEM_LIBRARIES__
        { LUA_IOLIBNAME, luaopen_io },
        { LUA_OSLIBNAME, luaopen_os },
#endif
        { LUA_STRLIBNAME, luaopen_string },
        { LUA_MATHLIBNAME, luaopen_math },
        { LUA_UTF8LIBNAME, luaopen_utf8 },
#ifdef DEBUG
        { LUA_DBLIBNAME, luaopen_debug },
#endif
        { NULL, NULL }
    };
    // "require" is different from preload in the sense that is also make the
    // library-module ready to be used (i.e. defined in the global space).
    for (const luaL_Reg *library = libraries; library->func; ++library) {
        luaL_requiref(L, library->name, library->func, 1);
        lua_pop(L, 1); // Remove the library (table) from the stack.
    }
}

void luaX_preload(lua_State *L, const char *modname, lua_CFunction loadf, int nup)
{
    luaL_getsubtable(L, LUA_REGISTRYINDEX, LUA_PRELOAD_TABLE);
    lua_insert(L, -(nup + 1)); // Move the `_PRELOAD` table above the upvalues.
    lua_pushcclosure(L, loadf, nup); // Closure with the upvalues (they are consumed)
    lua_setfield(L, -2, modname);
    lua_pop(L, 1); // Pop the `_PRELOAD` table
}

void luaX_requiref(lua_State *L, const char *modname, lua_CFunction openf, int nup, int glb)
{
    luaL_getsubtable(L, LUA_REGISTRYINDEX, LUA_LOADED_TABLE);
    lua_getfield(L, -1, modname); /* LOADED[modname] */
    if (!lua_toboolean(L, -1)) { /* package not already loaded? */
        lua_pop(L, 1); /* remove field */
        for (int i = 0; i < nup; ++i) { // Copy the upvalues to the top
            lua_pushvalue(L, -(nup + 1));
        }
        lua_pushcclosure(L, openf, nup); // Closure with those upvalues (the one just pushed will be removed)
        lua_pushstring(L, modname);      /* argument to open function */
        lua_call(L, 1, 1);               /* call 'openf' to open module */
        lua_pushvalue(L, -1);            /* make copy of module (call result) */
        lua_setfield(L, -3, modname);    /* LOADED[modname] = module */
    }
    lua_remove(L, -2); /* remove LOADED table */
    lua_insert(L, -(nup + 1)); // Move the module table above the upvalues.
    lua_pop(L, nup); // Pop the upvalues
    if (glb) {
        lua_pushvalue(L, -1);      /* copy of module */
        lua_setglobal(L, modname); /* _G[modname] = module */
    }
}

luaX_Reference luaX_ref(lua_State *L, int idx)
{
    lua_pushvalue(L, idx);
    return luaL_ref(L, LUA_REGISTRYINDEX);
}

void luaX_unref(lua_State *L, luaX_Reference ref)
{
    luaL_unref(L, LUA_REGISTRYINDEX, ref);
}

void luaX_checkargument(lua_State *L, int idx, const char *file, int line, const int types[])
{
    int actual_type = lua_type(L, idx);
    for (int i = 0; types[i] != LUAX_EOD; ++i) {
        int type = types[i];
        if (actual_type == type) { // Bail out if we match a type!
            return;
        }
    }
    luaL_error(L, "[%s:%d] signature failure for argument #%d (wrong actual type, got `%s`)", file, line, idx, lua_typename(L, actual_type));
}

int luaX_hassignature(lua_State *L, const int signature[])
{
    int argc = lua_gettop(L);

    int matched = 0;

    int idx = 1; // Lua's stack isn't zero-based.
    for (int i = 0; signature[i] != LUAX_EOD; ++i) {
        if (i == argc) { // Actual arguments are fewer than signature's formal ones. Bail out as not matching!
            return 0;
        }
        int type = signature[i];
        int actual_type = lua_type(L, idx++);
        if (actual_type != type && type != LUAX_ANY) { // Non matching argument! Bail out as not matching!
            return 0;
        }
        ++matched;
    }

    return matched == argc; // We need to matched the exact count of actual arguments. Having `countof(signature)` would've be easier.
}

int luaX_pushupvalues(lua_State *L)
{
    int nup = 0;
    for (int i = 1; ; ++i) {
        int idx = lua_upvalueindex(i);
        if (lua_isnone(L, idx)) {
            break;
        }
        lua_pushvalue(L, idx);
        ++nup;
    }
    return nup;
}

void luaX_pushvalues(lua_State *L, int nup)
{
    for (int i = 0; i < nup; ++i) {
        lua_pushvalue(L, -nup);
    }
}

int luaX_upvaluescount(lua_State *L)
{
    int nup = 0;
    for (int idx = 1; ; ++idx) {
        if (lua_isnone(L, lua_upvalueindex(idx))) {
            break;
        }
        ++nup;
    }
    return nup;
}
