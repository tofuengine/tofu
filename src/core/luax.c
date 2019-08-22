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
    const char *current = lua_tostring( L, -1 ); // grab path string from top of stack

    char *final = calloc(strlen(current) + 1 + strlen(path) + 5 + 1, sizeof(char));
    strcat(final, current);
    strcat(final, ";");
    strcat(final, path);
    strcat(final, "?.lua");

    lua_pop(L, 1); // get rid of the string on the stack we just pushed on line 5
    lua_pushstring(L, final); // push the new one
    lua_setfield(L, -2, "path"); // set the field "path" in table at -2 with value at top of stack
    lua_pop(L, 1); // get rid of package table from top of stack
}

int luaX_newmodule(lua_State *L, const char *script, const luaL_Reg *f, const luaX_Const *c, int nup, const char *name)
{
    if (script) {
        luaL_loadstring(L, script);
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

void luaX_preload(lua_State *L, const char *name, lua_CFunction f, int nup)
{
    lua_getglobal(L, "package");
    lua_getfield(L, -1, "preload");
    for (int i = 0; i < nup; ++i) { // Copy the upvalues to the top
        lua_pushvalue(L, -(nup + 2));
    }
    lua_pushcclosure(L, f, nup); // Closure with those upvalues (the one just pushed will be removed)
    lua_setfield(L, -2, name);
    lua_pop(L, nup + 2); // Pop the upvalues and the "package.preload" pair
}

int luaX_toref(lua_State *L, int arg)
{
    lua_pushvalue(L, arg);
    return luaL_ref(L, LUA_REGISTRYINDEX);
}

void luaX_getnumberarray(lua_State *L, int idx, double *array)
{
    int j = 0;
    lua_pushnil(L); // first key
    while (lua_next(L, idx)) {
#if 0
        const char *key_type = lua_typename(L, lua_type(L, -2)); // uses 'key' (at index -2) and 'value' (at index -1)
#endif
        array[j++] = lua_tonumber(L, -1);

        lua_pop(L, 1); // removes 'value'; keeps 'key' for next iteration
    }
}

void luaX_checkargument(lua_State *L, int index, const char *file, int line, ...)
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
        success |= func(L, index);
    }
    va_end(args);
    if ((count > 0) && !success) {
        luaL_error(L, "[%s:%d] signature failure for argument #%d w/ type %d", file, line, index, lua_type(L, index));
        return; // Unreachable code.
    }
}

extern int luaX_isnil(lua_State *L, int index)
{
    return lua_isnil(L, index);
}

extern int luaX_isboolean(lua_State *L, int index)
{
    return lua_isboolean(L, index);
}

int luaX_isinteger(lua_State *L, int index)
{
    return lua_isinteger(L, index);
}

int luaX_isnumber(lua_State *L, int index)
{
    return lua_isnumber(L, index);
}

int luaX_isstring(lua_State *L, int index)
{
    return lua_isstring(L, index);
}

int luaX_istable(lua_State *L, int index)
{
    return lua_istable(L, index);
}

int luaX_isfunction(lua_State *L, int index)
{
    return lua_isfunction(L, index);
}

int luaX_iscfunction(lua_State *L, int index)
{
    return lua_iscfunction(L, index);
}

int luaX_islightuserdata(lua_State *L, int index)
{
    return lua_islightuserdata(L, index);
}

int luaX_isuserdata(lua_State *L, int index)
{
    return lua_isuserdata(L, index);
}

int luaX_isthread(lua_State *L, int index)
{
    return lua_isthread(L, index);
}

int luaX_isany(lua_State *L, int index)
{
    return !lua_isnil(L, index);
}
