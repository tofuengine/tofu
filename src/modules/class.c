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

#include "class.h"

#include "../core/luax.h"

#include "../log.h"

static const char class_script[] =
    "local Class = {}\n"
    "\n"
    "function Class.define(model)\n"
    "  local proto = {}\n"
    "  -- If a base class is defined, the copy all the functions.\n"
    "  --\n"
    "  -- This is an instant snapshot, any new field defined runtime in the base\n"
    "  -- class won't be visible in the derived class.\n"
    "  if model then\n"
    "    Class.implement(proto, model)\n"
    "  end\n"
    "  -- This is the standard way in Lua to implement classes.\n"
    "  proto.__index = proto\n"
    "  proto.new = function(...)\n"
    "      local self = setmetatable({}, proto)\n"
    "      if self.__ctor then\n"
    "        self:__ctor(...)\n"
    "      end\n"
    "      return self\n"
    "    end\n"
    "  return proto\n"
    "end\n"
    "\n"
    "function Class.implement(proto, model)\n"
    "  for key, value in pairs(model) do\n"
    "    if type(value) == 'function' then\n"
    "      proto[key] = value\n"
    "    end\n"
    "  end\n"
    "end\n"
    "\n"
    "return Class\n"
;

int class_loader(lua_State *L)
{
    return luaX_newmodule(L, class_script, NULL, NULL, 0, NULL);
}
