/*
 * Copyright (c) 2019 Marco Lizza (marco.lizza@gmail.com)
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
 **/

#include "luax.h"

#include <stdlib.h>
#include <string.h>
#ifdef DEBUG
  #include <stb/stb_leakcheck.h>
#endif

typedef int (*luaX_TFunction)(lua_State *, int);

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

void luaX_stackdump(lua_State *L, const char* func, int line)
{
    int top = lua_gettop(L);
    printf("----------[ STACK DUMP (%s:%d) top=%d ]----------\n", func, line, top);
    for (int i = 0; i < top; i++) {
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
                    printf("\t%p", lua_tocfunction(L, positive));
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
        }
        printf("\n");
    }
}

// We also could have used the "LUA_PATH" environment variable.
void luaX_appendpath(lua_State *L, const char *path)
{
    lua_getglobal(L, "package");
    lua_getfield(L, -1, "path"); // get field "path" from table at top of stack (-1)

    const char *current = lua_tostring(L, -1); // grab path string from top of stack
    size_t length = strlen(current) + 1 + strlen(path) + 5; // <current>;<path>?.lua
    char *fullpath = malloc((length + 1) * sizeof(char));
    strcpy(fullpath, current);
    strcat(fullpath, ";");
    strcat(fullpath, path);
    strcat(fullpath, "?.lua");

    lua_pushstring(L, fullpath); // push the new one
    lua_setfield(L, -3, "path"); // set the field "path" in table at -2 with value at top of stack

    free(fullpath);

    lua_pop(L, 2); // get rid of package table from top of stack
}

void luaX_overridesearchers(lua_State *L, lua_CFunction searcher, int nup)
{
    lua_getglobal(L, "package"); // Access the `package.searchers` table.
    lua_getfield(L, -1, "searchers");

    for (int i = 0; i < nup; ++i) {
        lua_pushvalue(L, -(2 + nup));
    }
    lua_pushcclosure(L, searcher, 1);
    lua_rawseti(L, -2, 2); // Override the 2nd searcher (keep the "preloaded" helper).

    int n = lua_rawlen(L, -1);
    for (int i = 3; i <= n; ++i) { // Discard the others (two) searchers.
        lua_pushnil(L);
        lua_rawseti(L, -2, i);
    }

    lua_pop(L, 2 + nup); // Pop the `package` and `searchers` table, and consume the upvalues.
}

int luaX_newmodule(lua_State *L, const luaX_Script *script, const luaL_Reg *f, const luaX_Const *c, int nup, const char *name)
{
    if (script && script->data && script->length > 0) {
        luaL_loadbuffer(L, script->data, script->length, script->name);
        lua_pcall(L, 0, LUA_MULTRET, 0); // Just the export table is returned.
        if (name) {
            lua_pushstring(L, name);
            lua_setfield(L, -2, "__name");  /* metatable.__name = tname */
            lua_pushvalue(L, -1);
            lua_setfield(L, LUA_REGISTRYINDEX, name);  /* registry.name = metatable */
        }
    } else {
        luaL_newmetatable(L, name); /* create metatable */
    }

    // Duplicate the metatable, since it will be popped by the 'lua_setfield()' call.
    // This is equivalent to the following in lua:
    // metatable = {}
    // metatable.__index = metatable
    if (name) {
        lua_pushvalue(L, -1); // Possibly redundant, if already done in the script.
        lua_setfield(L, -2, "__index");  /* metatable.__index = metatable */
    }

    if (f) {
        for (int i = 0; i < nup; ++i) { // Duplicate upvalues (take a "+ 1" into account to skip the table).
            lua_pushvalue(L, -(nup + 1));
        }
        luaL_setfuncs(L, f, nup); // Register the function into the table at the top of the stack, i.e. create the methods
    }

    if (c) {
        for (; c->name; c++) {
            switch (c->type) {
                case LUA_CT_BOOLEAN: { lua_pushboolean(L, c->value.b); } break;
                case LUA_CT_INTEGER: { lua_pushinteger(L, c->value.i); } break;
                case LUA_CT_NUMBER: { lua_pushnumber(L, c->value.n); } break;
                case LUA_CT_STRING: { lua_pushstring(L, c->value.sz); } break;
            }
            lua_setfield(L, -2, c->name);
        }
    }

    // We need to return the module table on top of the stack upon return. Since it's a common idiom that upvalues are
    // consumed by the called function, we move the table "under" the upvalues and pop them.
    lua_insert(L, -(nup + 1));
    lua_pop(L, nup);

    return 1;
}

void luaX_preload(lua_State *L, const char *modname, lua_CFunction loadf, int nup)
{
    lua_getglobal(L, "package");
    lua_getfield(L, -1, "preload");
    for (int i = 0; i < nup; ++i) { // Copy the upvalues to the top
        lua_pushvalue(L, -(nup + 2));
    }
    lua_pushcclosure(L, loadf, nup); // Closure with those upvalues (the one just pushed will be removed)
    lua_setfield(L, -2, modname);
    lua_pop(L, nup + 2); // Pop the upvalues and the "package.preload" pair
}

void luaX_require(lua_State *L, const char *modname, lua_CFunction openf, int nup, int glb)
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
    if (glb) {
        lua_pushvalue(L, -1);      /* copy of module */
        lua_setglobal(L, modname); /* _G[modname] = module */
    }
    lua_pop(L, nup); // Pop the upvalues
}

int luaX_toref(lua_State *L, int arg)
{
    lua_pushvalue(L, arg);
    return luaL_ref(L, LUA_REGISTRYINDEX);
}

void luaX_pushnumberarray(lua_State *L, float *array, size_t count)
{
    lua_createtable(L, count, 0);
    for (size_t i = 0; i < count; i++) {
        lua_pushnumber(L, (double)array[i]);
        lua_rawseti(L, -2, i + 1); /* In lua indices start at 1 */
    }
}

void luaX_tonumberarray(lua_State *L, int idx, float *array, size_t count)
{
    int ref = (idx > 0) ? idx : idx - 1; // If negative, adjust to skip the iteration key.
    lua_pushnil(L);
    for (size_t i = 0; lua_next(L, ref); ++i) {
        if (i >= count) {
            lua_pop(L, 2); // Pops both key and value and bail out!
            break;
        }
        array[i] = (float)lua_tonumber(L, -1);
        lua_pop(L, 1);
    }
}

void luaX_checkargument(lua_State *L, int idx, const char *file, int line, ...)
{
    int count = 0;
    int success = 0;
    va_list args;
    va_start(args, line);
    for (; ; ++count) {
        luaX_TFunction func = va_arg(args, luaX_TFunction);
        if (!func) {
            break;
        }
        success |= func(L, idx);
    }
    va_end(args);
    if ((count > 0) && !success) {
        luaL_error(L, "[%s:%d] signature failure for argument #%d w/ type %d", file, line, idx, lua_type(L, idx));
        return; // Unreachable code.
    }
}

size_t luaX_packupvalues(lua_State *L, int nup)
{
    lua_pushinteger(L, nup); // Pass the upvalues count, for later usage.
    for (int j = 0; j < nup; ++j) {
        lua_pushvalue(L, -(nup + 1)); // Copy the upvalue, skipping the counter.
    }
    return nup + 1;
}

size_t luaX_unpackupvalues(lua_State *L)
{
    if (!lua_isinteger(L, lua_upvalueindex(1))) { // Check if the 1st upvalue is defined and is an integer.
        return 0;
    }
    int nup = lua_tointeger(L, lua_upvalueindex(1)); // The first upvalue tells how many upvalues we are handling.
    for (int i = 0; i< nup; ++i) {
        lua_pushvalue(L, lua_upvalueindex(2 + i)); // Copy the upvalues onto the stack, skipping the counter.
    }
    return (size_t)nup;
}

size_t luaX_count(lua_State *L, int idx)
{
    size_t count = 0;
    lua_pushnil(L); // first key
    while (lua_next(L, idx)) {
        count += 1;
        lua_pop(L, 1); // removes 'value'; keeps 'key' for next iteration
    }
    return count;
}

extern int luaX_isnil(lua_State *L, int idx)
{
    return lua_isnil(L, idx);
}

extern int luaX_isboolean(lua_State *L, int idx)
{
    return lua_isboolean(L, idx);
}

int luaX_isinteger(lua_State *L, int idx)
{
    return lua_isinteger(L, idx);
}

int luaX_isnumber(lua_State *L, int idx)
{
    return lua_isnumber(L, idx);
}

int luaX_isstring(lua_State *L, int idx)
{
    return lua_isstring(L, idx);
}

int luaX_istable(lua_State *L, int idx)
{
    return lua_istable(L, idx);
}

int luaX_isfunction(lua_State *L, int idx)
{
    return lua_isfunction(L, idx);
}

int luaX_iscfunction(lua_State *L, int idx)
{
    return lua_iscfunction(L, idx);
}

int luaX_islightuserdata(lua_State *L, int idx)
{
    return lua_islightuserdata(L, idx);
}

int luaX_isuserdata(lua_State *L, int idx)
{
    return lua_isuserdata(L, idx);
}

int luaX_isthread(lua_State *L, int idx)
{
    return lua_isthread(L, idx);
}

int luaX_isany(lua_State *L, int idx)
{
    return !lua_isnil(L, idx);
}
