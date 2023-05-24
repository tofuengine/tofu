--[[
MIT License

Copyright (c) 2019-2023 Marco Lizza

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
]]--

local Class = {}

function Class.define()
  local proto <const> = {}
  proto.__index = proto
  proto.new = function(...)
      local self <const> = setmetatable({}, proto)
      if self.__ctor then
        self:__ctor(...)
      end
      return self
    end
  return proto
end

function Class.borrow(proto, model, criteria)
  for id, other in pairs(model) do
    if type(other) == "function" and id ~= "new" then -- Skip the `new()` static method.
      local this <const> = proto[id]
      if this and (id == "__ctor" or criteria == "extend") then -- `__ctor()` is always extended.
        proto[id] = function(...)
            this(...)
            other(...)
          end
      elseif this and criteria == "chain" then
        proto[id] = function(...)
            other(...)
            this(...)
          end
      elseif not this or criteria == "replace" then
        proto[id] = other
      end
    end
  end
end

function Class.memoize(f)
  return setmetatable({}, {
      __index = function(self, k)
            local v <const> = f(k);
            self[k] = v
            return v;
        end
    });
end

return Class
