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
 * Copyright (c) 2019-2026 Marco Lizza
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

#if !defined(LUAX_RUNTIME_CHECKS) && defined(DEBUG)
    #define LUAX_RUNTIME_CHECKS
#endif

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
#define LUA_TLOBJECT    LUA_TTABLE

#define LUAX_NIL_INDEX    (0)
#define LUAX_NIL_TABLE    LUAX_NIL_INDEX
#define LUAX_NIL_FUNCTION LUAX_NIL_INDEX

#if defined(LUAX_RUNTIME_CHECKS)
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

// We can either check overloads by arity only, or by types. The latter is
// the default.
//
// Checking by arity is simpler and faster, but less flexible (e.g. it doesn't
// allow multiple overloads with the same arity, but different types). Checking
// by types is more flexible, but also more complex and slower (it has to check
// the types of the arguments at runtime, which can be expensive, and requires
// the usage of `if-else` chains instead of a `switch` statement).
//
// In practice, type-overloading is used only in the constructors of our
// classes, which are called only a few times during the program execution.
// This means the the overall impact is negligible, and the flexibility is worth
// the cost.
#if defined(LUAX_ARITY_OVERLOAD_ONLY)
    #define LUAX_OVERLOAD_BEGIN(L) \
        do { \
            lua_State *_L = (L); \
            int _argc = lua_gettop(_L); \
            switch (_argc) {
    #define LUAX_OVERLOAD_BY_ARITY(f, n) \
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
    #define LUAX_OVERLOAD_BY_ARITY(f, n) \
            if (_argc == (n)) { return (f)(_L); } else
    #define LUAX_OVERLOAD_BY_TYPES(f, ...) \
            if (luaX_hassignature(_L, (const int[]){ __VA_ARGS__, LUA_TEOD })) { return (f)(_L); } else
    #define LUAX_OVERLOAD_END \
            { return luaL_error(L, "[%s:%d] overload for arity #%d is missing", __FILE__, __LINE__, _argc); } \
        } while (0);
#endif

// Defining `LUAX_STRICT_INTEGERS` makes `LUAX_INTEGER` and `LUAX_UNSIGNED` check
// for integer-ness of the argument, instead of just number-ness. This is useful
// to catch bugs where a float is passed instead of an integer, but it can be too
// strict in some cases (it requires a lot of boilerplate number-to-integer code
// in the Lua scripts, when integer coercion is often acceptable), so it's
// disabled by default.
#define LUAX_IS_BOOLEAN(L, idx)     (lua_isboolean((L), (idx)))
#if defined(LUAX_STRICT_INTEGERS)
#define LUAX_IS_INTEGER(L, idx)     (lua_isinteger((L), (idx)))
#define LUAX_IS_UNSIGNED(L, idx)    (lua_isinteger((L), (idx)))
#else
#define LUAX_IS_INTEGER(L, idx)     (lua_isnumber((L), (idx)))
#define LUAX_IS_UNSIGNED(L, idx)    (lua_isnumber((L), (idx)))
#endif
#define LUAX_IS_NUMBER(L, idx)      (lua_isnumber((L), (idx)))
#define LUAX_IS_STRING(L, idx)      (lua_isstring((L), (idx)))
#define LUAX_IS_LSTRING(L, idx)     (lua_isstring((L), (idx)))
#define LUAX_IS_ENUM(L, idx)        (luaX_isenum((L), (idx)))
#define LUAX_IS_TABLE(L, idx)       (lua_istable((L), (idx)))
#define LUAX_IS_USERDATA(L, idx)    (lua_isuserdata((L), (idx)))
#define LUAX_IS_OBJECT(L, idx, t)   (luaX_isobject((L), (idx), (t)))
#define LUAX_IS_FUNCTION(L, idx)    (lua_isfunction((L), (idx)))

#if defined(LUAX_RUNTIME_CHECKS)
    #define LUAX_BOOLEAN(L, idx)                  (!LUAX_IS_BOOLEAN((L), (idx)) ? luaL_error((L), "argument #%d has wrong type", (idx)), 0 : lua_toboolean((L), (idx)))
    #define LUAX_INTEGER(L, idx)                  (!LUAX_IS_INTEGER((L), (idx)) ? luaL_error((L), "argument #%d has wrong type", (idx)), 0 : lua_tointeger((L), (idx)))
    #define LUAX_INTEGER_RANGE(L, idx, min, max)  (!LUAX_IS_INTEGER((L), (idx)) || !luaX_isinrangei((L), (idx), (min), (max)) ? luaL_error((L), "argument #%d is out of range [%d,%d]", (idx), (min), (max)), 0 : LUAX_INTEGER((L), (idx)))
    #define LUAX_UNSIGNED(L, idx)                 (!LUAX_IS_UNSIGNED((L), (idx)) ? luaL_error((L), "argument #%d has wrong type", (idx)), 0 : (lua_Unsigned)lua_tointeger((L), (idx)))
    #define LUAX_UNSIGNED_RANGE(L, idx, min, max) (!LUAX_IS_UNSIGNED((L), (idx)) || !luaX_isinrangeu((L), (idx), (min), (max)) ? luaL_error((L), "argument #%d is out of range [%d,%d]", (idx), (min), (max)), 0 : LUAX_UNSIGNED((L), (idx)))
    #define LUAX_NUMBER(L, idx)                   (!LUAX_IS_NUMBER((L), (idx)) ? luaL_error((L), "argument #%d has wrong type", (idx)), 0.0f : lua_tonumber((L), (idx)))
    #define LUAX_NUMBER_RANGE(L, idx, min, max)   (!LUAX_IS_NUMBER((L), (idx)) || !luaX_isinrangen((L), (idx), (min), (max)) ? luaL_error((L), "argument #%d is out of range [%f,%f]", (idx), (min), (max)), 0 : LUAX_NUMBER((L), (idx)))
    #define LUAX_STRING(L, idx)                   (!LUAX_IS_STRING((L), (idx)) ? luaL_error((L), "argument #%d has wrong type", (idx)), NULL : lua_tostring((L), (idx)))
    #define LUAX_LSTRING(L, idx)                  (!LUAX_IS_LSTRING((L), (idx)) ? luaL_error((L), "argument #%d has wrong type", (idx)), (luaX_String){ 0 } : luaX_tolstring((L), (idx)))
    #define LUAX_ENUM(L, idx, ids)                (!LUAX_IS_ENUM((L), (idx)) ? luaL_error((L), "argument #%d has wrong type", (idx)), 0 : luaX_toenum((L), (idx), (ids)))
    #define LUAX_TABLE(L, idx)                    (!LUAX_IS_TABLE((L), (idx)) ? luaL_error((L), "argument #%d has wrong type", (idx)), LUAX_NIL_TABLE : luaX_totable((L), (idx)))
    #define LUAX_USERDATA(L, idx)                 (!LUAX_IS_USERDATA((L), (idx)) ? luaL_error((L), "argument #%d has wrong type", (idx)), NULL : lua_touserdata((L), (idx)))
    #define LUAX_OBJECT(l, idx, t)                (!LUAX_IS_OBJECT((l), (idx), (t)) ? luaL_error((l), "argument #%d has wrong type (expected #%d)", (idx), (t)), NULL : luaX_toobject((l), (idx), (t)))
    #define LUAX_FUNCTION(l, idx)                 (!LUAX_IS_FUNCTION((l), (idx)) ? luaL_error((l), "argument #%d has wrong type", (idx)), LUAX_NIL_FUNCTION : luaX_tofunction((l), (idx)))
#else
    #define LUAX_BOOLEAN(L, idx)                  (lua_toboolean((L), (idx)))
    #define LUAX_INTEGER(L, idx)                  (lua_tointeger((L), (idx)))
    #define LUAX_INTEGER_RANGE(L, idx, min, max)  (LUAX_INTEGER((L), (idx)))
    #define LUAX_UNSIGNED(L, idx)                 ((lua_Unsigned)lua_tointeger((L), (idx)))
    #define LUAX_UNSIGNED_RANGE(L, idx, min, max) (LUAX_UNSIGNED((L), (idx)))
    #define LUAX_NUMBER(L, idx)                   (lua_tonumber((L), (idx)))
    #define LUAX_NUMBER_RANGE(L, idx, min, max)   (LUAX_NUMBER((L), (idx)))
    #define LUAX_STRING(L, idx)                   (lua_tostring((L), (idx)))
    #define LUAX_LSTRING(L, idx)                  (luaX_tolstring((L), (idx)))
    #define LUAX_ENUM(L, idx, ids)                (luaX_toenum((L), (idx), (ids)))
    #define LUAX_TABLE(L, idx)                    (luaX_totable((L), (idx)))
    #define LUAX_USERDATA(L, idx)                 (lua_touserdata((L), (idx)))
    #define LUAX_OBJECT(l, idx, t)                (luaX_toobject((L), (idx), (t)))
    #define LUAX_FUNCTION(l, idx)                 (luaX_tofunction((L), (idx)))
#endif

#define LUAX_OPTIONAL_BOOLEAN(L, idx, def)                  (lua_isnoneornil((L), (idx)) ? (def) : LUAX_BOOLEAN((L), (idx)))
#define LUAX_OPTIONAL_INTEGER(L, idx, def)                  (lua_isnoneornil((L), (idx)) ? (def) : LUAX_INTEGER((L), (idx)))
#define LUAX_OPTIONAL_INTEGER_RANGE(L, idx, min, max, def)  (lua_isnoneornil((L), (idx)) ? (def) : LUAX_INTEGER_RANGE((L), (idx), (min), (max)))
#define LUAX_OPTIONAL_UNSIGNED(L, idx, def)                 (lua_isnoneornil((L), (idx)) ? (def) : LUAX_UNSIGNED((L), (idx)))
#define LUAX_OPTIONAL_UNSIGNED_RANGE(L, idx, min, max, def) (lua_isnoneornil((L), (idx)) ? (def) : LUAX_UNSIGNED_RANGE((L), (idx), (min), (max)))
#define LUAX_OPTIONAL_NUMBER(L, idx, def)                   (lua_isnoneornil((L), (idx)) ? (def) : LUAX_NUMBER((L), (idx)))
#define LUAX_OPTIONAL_NUMBER_RANGE(L, idx, min, max, def)   (lua_isnoneornil((L), (idx)) ? (def) : LUAX_NUMBER_RANGE((L), (idx), (min), (max)))
#define LUAX_OPTIONAL_STRING(L, idx, def)                   (lua_isnoneornil((L), (idx)) ? (def) : LUAX_STRING((L), (idx)))
#define LUAX_OPTIONAL_LSTRING(L, idx, def)                  (lua_isnoneornil((L), (idx)) ? (luaX_String){ .data = (def), .size = strlen((def)) } : LUAX_LSTRING((L), (idx)))
#define LUAX_OPTIONAL_ENUM(L, idx, ids, def)                (lua_isnoneornil((L), (idx)) ? (def) : LUAX_ENUM((L), (idx), (ids)))
#define LUAX_OPTIONAL_TABLE(L, idx, def)                    (lua_isnoneornil((L), (idx)) ? (def) : LUAX_TABLE((L), (idx)))
#define LUAX_OPTIONAL_USERDATA(L, idx, def)                 (lua_isnoneornil((L), (idx)) ? (def) : LUAX_USERDATA((L), (idx)))
#define LUAX_OPTIONAL_OBJECT(l, idx, t, def)                (lua_isnoneornil((L), (idx)) ? (def) : LUAX_OBJECT((L), (idx), (t)))
#define LUAX_OPTIONAL_FUNCTION(l, idx, def)                 (lua_isnoneornil((L), (idx)) ? (def) : LUAX_FUNCTION((L), (idx)))

// The following macros offers a simple way to check and validate if the Lua's
// VM stack is corrupted/unbalanced.
#if defined(LUAX_RUNTIME_CHECKS)
    #define LUAX_STACK_BEGIN(L) \
        do { \
            int _top = lua_gettop((L));
    #define LUAX_STACK_END(L, delta) \
            int _delta = lua_gettop((L)) - _top; \
            if (_delta != (delta)) { \
                return luaL_error(L, "[%s:%d] stack is unbalanced (%d)", __FILE__, __LINE__, _delta); \
            } \
        } while (0);
#else
    #define LUAX_STACK_BEGIN(L)
    #define LUAX_STACK_END(L, delta)
#endif

#define LUAX_UNUSED(x)   (void)(x)

#define luaX_dump(L) luaX_stackdump((L), __FILE__, __LINE__)

extern int luaX_isinrangei(lua_State *L, int idx, lua_Integer min, lua_Integer max);
extern int luaX_isinrangeu(lua_State *L, int idx, lua_Unsigned min, lua_Unsigned max);
extern int luaX_isinrangen(lua_State *L, int idx, lua_Number min, lua_Number max);

extern luaX_String luaX_tolstring(lua_State *L, int idx);

extern int luaX_isenum(lua_State *L, int idx);
extern int luaX_toenum(lua_State *L, int idx, const char **ids);

extern int luaX_totable(lua_State *L, int idx);
extern int luaX_tofunction(lua_State *L, int idx);

extern void *luaX_newobject(lua_State *L, size_t size, const void *state, int type, const char *metatable);
extern int luaX_isobject(lua_State *L, int idx, int type);
extern void *luaX_toobject(lua_State *L, int idx, int type);

extern void luaX_stackdump(lua_State *L, const char *file, int line);
extern void luaX_overridesearchers(lua_State *L, lua_CFunction searcher, int nup, int sandbox_mode);
extern int luaX_insisttable(lua_State *L, const char *name);
extern int luaX_newmodule(lua_State *L, luaX_Script script, const luaL_Reg *f, const luaX_Const *c, int nup, const char *name);
extern void luaX_preload(lua_State *L, const char *modname, lua_CFunction openf, int nup);

extern luaX_Reference luaX_ref(lua_State *L, int idx);
extern void luaX_unref(lua_State *L, luaX_Reference ref);
extern int luaX_pushref(lua_State *L, luaX_Reference ref);

extern void luaX_checkargument(lua_State *L, int idx, const char *file, int line, const int types[]);

extern int luaX_hassignature(lua_State *L, const int signature[]);

extern void luaX_pushvalues(lua_State *L, int nup);
extern int luaX_pushupvalues(lua_State *L);
extern int luaX_upvaluescount(lua_State *L); // UNUSED

extern int luaX_isgcrunning(lua_State *L);
extern void luaX_gccollect(lua_State *L);
extern void luaX_gccycle(lua_State *L, int max_steps);
extern int luaX_gcstep(lua_State *L);
extern int luaX_memoryusage(lua_State *L);

#endif  /* TOFU_LIBS_LUAX_H */
