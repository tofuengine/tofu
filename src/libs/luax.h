/*
 * MIT License
 * 
 * Copyright (c) 2019-2023 Marco Lizza
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

#ifndef TOFU_LIBS_LUAX_H
#define TOFU_LIBS_LUAX_H

#include <lua/lua.h>
#include <lua/lualib.h>
#include <lua/lauxlib.h>

typedef enum luaX_Const_Type_e {
    LUA_CT_NIL,
    LUA_CT_BOOLEAN,
    LUA_CT_INTEGER,
    LUA_CT_NUMBER,
    LUA_CT_STRING
} luaX_Const_Type;

typedef struct luaX_Const_s {
    const char *name;
    luaX_Const_Type type;
    union {
        int b;
        lua_Integer i;
        lua_Number n;
        const char *sz;
    } value;
} luaX_Const;

typedef struct luaX_Script_s {
    const char *data;
    size_t size;
    const char *name;
} luaX_Script;

typedef int luaX_Reference;

typedef struct luaX_String_s {
    const char *data;
    size_t size;
} luaX_String;

//#define LUA_TNIL      0
//#define LUA_TNONE     (-1)
#define LUA_TANY        (-2)
#define LUA_TEOD        (-3)
#define LUA_TENUM       LUA_TSTRING
#define LUA_TOBJECT     LUA_TUSERDATA

#if defined(DEBUG)
    #define LUAX_SIGNATURE_BEGIN(L) \
        do { \
            lua_State *_L = (L); \
            int _argc = lua_gettop(_L); \
            int _matched = 0; \
            int _index = 1;
    #define LUAX_SIGNATURE_REQUIRED(...) \
            luaX_checkargument(_L, _index++, __FILE__, __LINE__, (const int[]){ __VA_ARGS__, LUA_TEOD }); \
            ++_matched;
    #define LUAX_SIGNATURE_OPTIONAL(...) \
            luaX_checkargument(_L, _index, __FILE__, __LINE__, (const int[]){ __VA_ARGS__, LUA_TNIL, LUA_TNONE, LUA_TEOD }); \
            if (!lua_isnone(_L, _index++)) { \
                ++_matched; \
            }
    #define LUAX_SIGNATURE_END \
            if (_matched != _argc) { \
                luaL_error(_L, "[%s:%d] arguments number mismatch (checked %d, matched %d out of %d)", __FILE__, __LINE__, _index - 1, _matched, _argc); \
            } \
        } while (0);
#else
    #define LUAX_SIGNATURE_BEGIN(l)
    #define LUAX_SIGNATURE_REQUIRED(...)
    #define LUAX_SIGNATURE_OPTIONAL(...)
    #define LUAX_SIGNATURE_END
#endif

#if defined(__LUAX_ARITY_OVERLOAD_ONLY__)
    #define LUAX_OVERLOAD_BEGIN(L) \
        do { \
            lua_State *_L = (L); \
            int _argc = lua_gettop(_L); \
            switch (_argc) {
    #define LUAX_OVERLOAD_ARITY(n, f) \
                case (n): { return (f)(_L); }
    #define LUAX_OVERLOAD_END \
                default: { return luaL_error(L, "[%s:%d] overload for arity #%d is missing", __FILE__, __LINE__, _argc); } \
            } \
        } while (0);
#else
    #define LUAX_OVERLOAD_BEGIN(L) \
        do { \
            lua_State *_L = (L); \
            int _argc = lua_gettop(_L);
    #define LUAX_OVERLOAD_ARITY(n, f) \
            if (_argc == (n)) { return (f)(_L); } else
    #define LUAX_OVERLOAD_SIGNATURE(f, ...) \
            if (luaX_hassignature(_L, (const int []){ __VA_ARGS__, LUA_TEOD })) { return (f)(_L); } else
    #define LUAX_OVERLOAD_END \
            { return luaL_error(L, "[%s:%d] overload for arity #%d is missing", __FILE__, __LINE__, _argc); } \
        } while (0);
#endif

#if defined(DEBUG)
    #define LUAX_BOOLEAN(L, idx)                 (!lua_isboolean((L), (idx)) ? luaL_error((L), "argument #%d has wrong type", (idx)), 0 : lua_toboolean((L), (idx)))
    #define LUAX_OPTIONAL_BOOLEAN(L, idx, def)   (lua_isnoneornil((L), (idx)) ? (def) : lua_toboolean((L), (idx)))
    #define LUAX_INTEGER(L, idx)                 (!lua_isnumber((L), (idx)) ? luaL_error((L), "argument #%d has wrong type", (idx)), 0 : lua_tointeger((L), (idx)))
    #define LUAX_OPTIONAL_INTEGER(L, idx, def)   (lua_isnoneornil((L), (idx)) ? (def) : lua_tointeger((L), (idx)))
    #define LUAX_UNSIGNED(L, idx)                (!lua_isnumber((L), (idx)) ? luaL_error((L), "argument #%d has wrong type", (idx)), 0 : (lua_Unsigned)lua_tointeger((L), (idx)))
    #define LUAX_OPTIONAL_UNSIGNED(L, idx, def)  (lua_isnoneornil((L), (idx)) ? (def) : (lua_Unsigned)lua_tointeger((L), (idx)))
    #define LUAX_NUMBER(L, idx)                  (!lua_isnumber((L), (idx)) ? luaL_error((L), "argument #%d has wrong type", (idx)), 0.0f : lua_tonumber((L), (idx)))
    #define LUAX_OPTIONAL_NUMBER(L, idx, def)    (lua_isnoneornil((L), (idx)) ? (def) : lua_tonumber((L), (idx)))
    #define LUAX_STRING(L, idx)                  (!lua_isstring((L), (idx)) ? luaL_error((L), "argument #%d has wrong type", (idx)), NULL : lua_tostring((L), (idx)))
    #define LUAX_OPTIONAL_STRING(L, idx, def)    (lua_isnoneornil((L), (idx)) ? (def) : lua_tostring((L), (idx)))
    #define LUAX_LSTRING(L, idx)                 (!lua_isstring((L), (idx)) ? luaL_error((L), "argument #%d has wrong type", (idx)), (luaX_String){ 0 } : luaX_tolstring((L), (idx)))
    #define LUAX_OPTIONAL_LSTRING(L, idx, def)   (lua_isnoneornil((L), (idx)) ? (luaX_String){ .data = (def), .size = strlen((def)) } : lua_tolstring((L), (idx)))
    #define LUAX_ENUM(L, idx, ids)               (!luaX_isenum((L), (idx)) ? luaL_error((L), "argument #%d has wrong type", (idx)), 0 : luaX_toenum((L), (idx), (ids)))
    #define LUAX_OPTIONAL_ENUM(L, idx, ids, def) (lua_isnoneornil((L), (idx)) ? (def) : luaX_toenum((L), (idx), (ids)))
    #define LUAX_TABLE(L, idx)                   (!lua_istable((L), (idx)) ? luaL_error((L), "argument #%d has wrong type", (idx)), NULL : lua_rawlen((L), (idx)))
    #define LUAX_OPTIONAL_TABLE(L, idx, def)     (lua_isnoneornil((L), (idx)) ? (def) : lua_rawlen((L), (idx)))
    #define LUAX_USERDATA(L, idx)                (!lua_isuserdata((L), (idx)) ? luaL_error((L), "argument #%d has wrong type", (idx)), NULL : lua_touserdata((L), (idx)))
    #define LUAX_OPTIONAL_USERDATA(L, idx, def)  (lua_isnoneornil((L), (idx)) ? (def) : lua_touserdata((L), (idx)))
    #define LUAX_OBJECT(l, idx, t)               (!luaX_isobject((L), (idx), (t)) ? luaL_error((L), "argument #%d has wrong type (expected #%d)", (idx), (t)), NULL : luaX_toobject((L), (idx), (t)))
    #define LUAX_OPTIONAL_OBJECT(l, idx, t, def) (lua_isnoneornil((L), (idx)) ? (def) : luaX_toobject((L), (idx), (t)))
#else
    #define LUAX_BOOLEAN(L, idx)                 (lua_toboolean((L), (idx)))
    #define LUAX_OPTIONAL_BOOLEAN(L, idx, def)   (lua_isnoneornil((L), (idx)) ? (def) : lua_toboolean((L), (idx)))
    #define LUAX_INTEGER(L, idx)                 (lua_tointeger((L), (idx)))
    #define LUAX_OPTIONAL_INTEGER(L, idx, def)   (lua_isnoneornil((L), (idx)) ? (def) : lua_tointeger((L), (idx)))
    #define LUAX_UNSIGNED(L, idx)                ((lua_Unsigned)lua_tointeger((L), (idx)))
    #define LUAX_OPTIONAL_UNSIGNED(L, idx, def)  (lua_isnoneornil((L), (idx)) ? (def) : (lua_Unsigned)lua_tointeger((L), (idx)))
    #define LUAX_NUMBER(L, idx)                  (lua_tonumber((L), (idx)))
    #define LUAX_OPTIONAL_NUMBER(L, idx, def)    (lua_isnoneornil((L), (idx)) ? (def) : lua_tonumber((L), (idx)))
    #define LUAX_STRING(L, idx)                  (lua_tostring((L), (idx)))
    #define LUAX_OPTIONAL_STRING(L, idx, def)    (lua_isnoneornil((L), (idx)) ? (def) : lua_tostring((L), (idx)))
    #define LUAX_LSTRING(L, idx)                 (luaX_tolstring((L), (idx)))
    #define LUAX_OPTIONAL_LSTRING(L, idx, def)   (lua_isnoneornil((L), (idx)) ? (luaX_String){ .data = (def), .size = strlen((def)) } : luaX_tolstring((L), (idx)))
    #define LUAX_ENUM(L, idx, ids)               (luaX_toenum((L), (idx), (ids)))
    #define LUAX_OPTIONAL_ENUM(L, idx, ids, def) (lua_isnoneornil((L), (idx)) ? (def) : luaX_toenum((L), (idx), (ids)))
    #define LUAX_TABLE(L, idx)                   (lua_rawlen((L), (idx)))
    #define LUAX_OPTIONAL_TABLE(L, idx, def)     (lua_isnoneornil((L), (idx)) ? (def) : lua_rawlen((L), (idx)))
    #define LUAX_USERDATA(L, idx)                (lua_touserdata((L), (idx)))
    #define LUAX_OPTIONAL_USERDATA(L, idx, def)  (lua_isnoneornil((L), (idx)) ? (def) : lua_touserdata((L), (idx)))
    #define LUAX_OBJECT(l, idx, t)               (luaX_toobject((L), (idx), (t)))
    #define LUAX_OPTIONAL_OBJECT(l, idx, t, def) (lua_isnoneornil((L), (idx)) ? (def) : luaX_toobject((L), (idx), (t)))
#endif

#define luaX_dump(L)                luaX_stackdump((L), __FILE__, __LINE__)

#define luaX_tofunction(L, idx)     luaX_ref((L), (idx))

extern luaX_String luaX_tolstring(lua_State *L, int idx);

extern int luaX_isenum(lua_State *L, int idx);
extern int luaX_toenum(lua_State *L, int idx, const char **ids);

extern void *luaX_newobject(lua_State *L, size_t size, void *state, int type, const char *metatable);
extern int luaX_isobject(lua_State *L, int idx, int type);
extern void *luaX_toobject(lua_State *L, int idx, int type);

extern void luaX_stackdump(lua_State *L, const char *file, int line);
extern void luaX_overridesearchers(lua_State *L, lua_CFunction searcher, int nup, int sandbox_mode);
extern int luaX_insisttable(lua_State *L, const char *name);
extern int luaX_newmodule(lua_State *L, luaX_Script script, const luaL_Reg *f, const luaX_Const *c, int nup, const char *name);
extern void luaX_openlibs(lua_State *L);
extern void luaX_preload(lua_State *L, const char *modname, lua_CFunction openf, int nup);

extern luaX_Reference luaX_ref(lua_State *L, int idx);
extern void luaX_unref(lua_State *L, luaX_Reference ref);
extern int luaX_pushref(lua_State *L, luaX_Reference ref);

extern void luaX_checkargument(lua_State *L, int idx, const char *file, int line, const int types[]);

extern int luaX_hassignature(lua_State *L, const int signature[]);

extern void luaX_pushvalues(lua_State *L, int nup);
extern int luaX_pushupvalues(lua_State *L);
extern int luaX_upvaluescount(lua_State *L); // UNUSED

#endif  /* TOFU_LIBS_LUAX_H */
