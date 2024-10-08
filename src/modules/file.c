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

#include "file.h"

#include "internal/udt.h"

#include <core/config.h>
#include <systems/storage.h>

static int file_load_1s_1s(lua_State *L);
static int file_store_2ss_0(lua_State *L);

int file_loader(lua_State *L)
{
    return udt_newmodule(L,
        (const struct luaL_Reg[]){
            // -- operations --
            { "load", file_load_1s_1s },
            { "store", file_store_2ss_0 },
            { NULL, NULL }
        },
        (const luaX_Const[]){
            { NULL, LUA_CT_NIL, { 0 } }
        });
}

static int file_load_1s_1s(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
    LUAX_SIGNATURE_END
    const char *name = LUAX_STRING(L, 1);

    Storage_t *storage = (Storage_t *)udt_get_userdata(L, USERDATA_STORAGE);

    const Storage_Resource_t *resource = Storage_load(storage, name, STORAGE_RESOURCE_BLOB);
    if (!resource) {
        return luaL_error(L, "can't load file `%s`", name);
    }
    lua_pushlstring(L, SR_BPTR(resource), SR_BSIZE(resource)); // Lua's strings can contain bytes.

    return 1;
}

static int file_store_2ss_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
    LUAX_SIGNATURE_END
    const char *name = LUAX_STRING(L, 1);
    luaX_String data = LUAX_LSTRING(L, 2);

    Storage_t *storage = (Storage_t *)udt_get_userdata(L, USERDATA_STORAGE);

    Storage_Resource_t resource = (Storage_Resource_t){
            // .name = name,
            .type = STORAGE_RESOURCE_BLOB,
            .var = {
                .blob = {
                    .ptr = (void *)data.data,
                    .size = data.size
                }
            }
        };

    bool stored = Storage_store(storage, name, &resource);
    if (!stored) {
        return luaL_error(L, "can't store file `%s`", name);
    }

    return 0;
}
