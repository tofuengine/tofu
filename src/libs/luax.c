/*
 *                 ___________________  _______________ ___
 *                 \__    ___/\_____  \ \_   _____/    |   \
 *                   |    |    /   |   \ |    __) |    |   /
 *                   |    |   /    |    \|     \  |    |  /
 *                   |____|   \_______  /\___  /  |______/
 *                                    \/     \/
 *         ___________ _______    ________.___ _______  ___________
 *         \_   _____/ \      \  /  _____/|   |\      \ \_   _____/
 *          |    __)_  /   |   \/   \  ___|   |/   |   \ |    __)_
 *          |        \/    |    \    \_\  \   /    |    \|        \
 *         /_______  /\____|__  /\______  /___\____|__  /_______  /
 *                 \/         \/        \/            \/        \
 *
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

#include "luax.h"

#include <string.h>

// Unless RTTI has been explicitly disabled, automaticaly enables it in the
// DEBUG build. Please note that it can be force-enabled anyway.
#if !defined(LUAX_NO_RTTI) && defined(DEBUG)
    #define _LUAX_RTTI
#endif  /* LUAX_NO_RTTI */

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

luaX_String luaX_tolstring(lua_State *L, int idx)
{
    luaX_String s;
    s.data = lua_tolstring(L, idx, &s.size);
    return s; // Too bad, we can't use compound-literals and designated initializers! :>
}

int luaX_isenum(lua_State *L, int idx)
{
    return lua_isstring(L, idx);
}

int luaX_toenum(lua_State *L, int idx, const char **ids)
{
    const char *value = lua_tostring(L, idx);
    if (!value) {
#if defined(DEBUG)
        return luaL_error(L, "value at argument #%d is null", idx), 0;
#else   /* DEBUG */
        return -1;
#endif  /* DEBUG */
    }
    size_t length = strlen(value) + 1; // The length of the string doesn't, we can use it!
    for (int i = 0; ids[i]; ++i) {
        if (memcmp(value, ids[i], length) == 0) { // Use `memcmp()` to optimized for speed.
            return i;
        }
    }
#if defined(DEBUG)
    return luaL_error(L, "argument #%d w/ value `%s` is not a valid enumeration", idx, value), -1;
#else   /* DEBUG */
    return -1;
#endif  /* DEBUG */
}

#if defined(_LUAX_RTTI)
typedef struct luaX_Object_s {
    int type;
} luaX_Object;
#else
typedef void *luaX_Object;
#endif  /* _LUAX_RTTI */

#if defined(_LUAX_RTTI)
    #define LUAX_OBJECT_SIZE(s)     (sizeof(luaX_Object) + (s))
    #define LUAX_OBJECT_SELF(o)     ((void *)((luaX_Object *)(o) + 1))
#else
    #define LUAX_OBJECT_SIZE(s)     (s)
    #define LUAX_OBJECT_SELF(o)     ((void *)(o))
#endif  /* _LUAX_RTTI */

void *luaX_newobject(lua_State *L, size_t size, void *state, int type, const char *metatable)
{
    luaX_Object *object = (luaX_Object *)lua_newuserdatauv(L, LUAX_OBJECT_SIZE(size), 1);
#if defined(_LUAX_RTTI)
    *object = (luaX_Object){
            .type = type
        };
#endif  /* _LUAX_RTTI */
    luaL_setmetatable(L, metatable);
    void *self = LUAX_OBJECT_SELF(object);
    memcpy(self, state, size);
    return self;
}

int luaX_isobject(lua_State *L, int idx, int type)
{
    luaX_Object *object = (luaX_Object *)lua_touserdata(L, idx); // `lua_touserdata` returns NULL if not userdata!
    if (!object) {
#if defined(DEBUG)
        return luaL_error(L, "object at argument #%d is null", idx), 0;
#else   /* DEBUG */
        return 0;
#endif  /* DEBUG */
    }
#if defined(_LUAX_RTTI)
    return object->type == type;
#else   /* _LUAX_RTTI */
    return 1;
#endif  /* _LUAX_RTTI */
}

void *luaX_toobject(lua_State *L, int idx, int type)
{
    luaX_Object *object = (luaX_Object *)lua_touserdata(L, idx);
    if (!object) {
#if defined(DEBUG)
        return luaL_error(L, "object at argument #%d is null", idx), NULL;
#else   /* DEBUG */
        return NULL;
#endif  /* DEBUG */
    }

#if defined(_LUAX_RTTI)
    if (object->type != type) {
#if defined(DEBUG)
        return luaL_error(L, "object at argument #%d has wrong type (expected %d, actual %d)", idx, type, object->type), NULL;
#else   /* DEBUG */
        return NULL;
#endif  /* DEBUG */
    }
#endif  /* _LUAX_RTTI */

    return LUAX_OBJECT_SELF(object);
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

// Lua default searchers are stored as four entries in the `package.searchers` table, as follows:
//
//   - a searcher that looks for a loader in the `package.preload` table,
//   - a searcher that looks for a loader as a Lua library,
//   - a searcher that looks for a loader as a C library,
//   - a searcher that looks for an all-in-one, combined, loader.
//
// In sandbox-mode this function modifies the table by clearing table entries #3 and #4. The first one is kept
// (to enable module reuse), and the second one is overwritten with the given `searcher`. As a result the
//  module loading process is confined to the custom searcher only.
//
// See: https://www.lua.org/manual/5.4/manual.html#pdf-package.searchers
void luaX_overridesearchers(lua_State *L, lua_CFunction searcher, int nup, int sandbox_mode)
{
    lua_getglobal(L, "package");        // A ... A -> A ... A T
    lua_getfield(L, -1, "searchers");   // A ... A T -> A ... A T T

    // Move the `searchers` and `package` tables *before* the upvalues so we can "close" them into
    // a function-closure. Then use the closure to override the 2nd searcher (keeping the "preloaded" helper).
    lua_insert(L, -(nup + 2));          // A ... A T T -> T A ... A T
    lua_insert(L, -(nup + 2));          // T A ... A T -> T T A ... A
    lua_pushcclosure(L, searcher, nup); // T T A ... A -> T T F
    lua_rawseti(L, -2, 2);              // T T F -> T T

    // Discard the others (two) searchers.
    if (sandbox_mode) {
        lua_pushnil(L);
        lua_rawseti(L, -2, 3); // package.searchers[3] = nil
        lua_pushnil(L);
        lua_rawseti(L, -2, 4); // package.searchers[4] = nil
    }

    lua_pop(L, 2); // Pop the `package` and `searchers` table.

    // Upvalues have already been consumed by `lua_pushcclosure()`. No need to clear the stack.
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
    if (script.data && script.size > 0) {
        luaL_loadbuffer(L, script.data, script.size, script.name);
        lua_pcall(L, 0, LUA_MULTRET, 0); // Just the export table is returned.
        if (name) {
            // Partially taken from `luaL_newmetatable()` in order to use the script return value (the export table)
            // as metatable (see https://www.lua.org/pil/28.2.html).
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

    lua_insert(L, -(nup + 1)); // Move the table above the upvalues (to permit them to be consumed while preserving the table).
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
#if !defined(LUAX_NO_SYSTEM_LIBRARIES)
        { LUA_IOLIBNAME, luaopen_io },
        { LUA_OSLIBNAME, luaopen_os },
#endif  /* LUAX_NO_SYSTEM_LIBRARIES */
        { LUA_STRLIBNAME, luaopen_string },
        { LUA_MATHLIBNAME, luaopen_math },
        { LUA_UTF8LIBNAME, luaopen_utf8 },
#if defined(DEBUG)
        { LUA_DBLIBNAME, luaopen_debug },
#endif  /* DEBUG */
        { NULL, NULL }
    };
    // "require" is different from preload in the sense that is also make the
    // library-module ready to be used (i.e. defined in the global space).
    for (const luaL_Reg *library = libraries; library->func; ++library) {
        luaL_requiref(L, library->name, library->func, 1);
        lua_pop(L, 1); // Remove the library (table) from the stack.
    }
}

// Preloading a Lua module from the FFI API is achieved by storing the a loader
// function in the `_PRELOAD` registry table. The module is not loaded, yet, but
// also prepared for later usage.
void luaX_preload(lua_State *L, const char *modname, lua_CFunction loadf, int nup)
{
    luaL_getsubtable(L, LUA_REGISTRYINDEX, LUA_PRELOAD_TABLE);
    lua_insert(L, -(nup + 1)); // Move the `_PRELOAD` table above the upvalues.
    lua_pushcclosure(L, loadf, nup); // Closure with the upvalues (they are consumed)
    lua_setfield(L, -2, modname);
    lua_pop(L, 1); // Pop the `_PRELOAD` table
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

int luaX_pushref(lua_State *L, luaX_Reference ref)
{
    return lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
}

void luaX_checkargument(lua_State *L, int idx, const char *file, int line, const int types[])
{
    int actual_type = lua_type(L, idx);
    for (int i = 0; types[i] != LUA_TEOD; ++i) {
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
    for (int i = 0; signature[i] != LUA_TEOD; ++i) {
        if (i == argc) { // Actual arguments are fewer than signature's formal ones. Bail out as not matching!
            return 0;
        }
        int type = signature[i];
        int actual_type = lua_type(L, idx++);
        if (actual_type != type && type != LUA_TANY) { // Non matching argument! Bail out as not matching!
            return 0;
        }
        ++matched;
    }

    return matched == argc; // We need to match the exact count of actual arguments. Having `countof(signature)` would've be easier.
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
