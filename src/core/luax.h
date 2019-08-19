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

#ifndef __CORE_LUAX_H__
#define __CORE_LUAX_H__

#include <lua/lua.h>
#include <lua/lualib.h>
#include <lua/lauxlib.h>

typedef enum _luaX_Const_Type {
    LUA_CT_BOOLEAN,
    LUA_CT_INTEGER,
    LUA_CT_NUMBER,
    LUA_CT_STRING,
} luaX_Const_Type;

typedef struct _luaX_Const {
    const char *name;
    luaX_Const_Type type;
    union {
        int b;
        lua_Integer i;
        lua_Number n;
        const char *sz;
    } value;
} luaX_Const;

#define LUAX_MODULE(n)          #n
#define LUAX_CLASS(n)           #n "_mt"

#define luaX_dump(L)   luaX_stackdump(L, __FILE__, __LINE__)

extern void luaX_stackdump(lua_State *L, const char* func, int line);
extern void luaX_appendpath(lua_State *L, const char *path);
extern int luaX_newmodule(lua_State *L, const char *script, const luaL_Reg *f, const luaX_Const *c, const char *name);
extern void luaX_preload(lua_State *L, const char *name, lua_CFunction f);
extern int luaX_checkfunction(lua_State *L, int arg);
extern void luaX_setuserdata(lua_State *L, const char *name, void *p);
extern void *luaX_getuserdata(lua_State *L, const char *name);
extern void luaX_getnumberarray(lua_State *L, int idx, double *array);
extern void luaX_setglobals(lua_State *L, const luaL_Reg *l, int nup);

#endif  /* __CORE_LUAX_H__ */