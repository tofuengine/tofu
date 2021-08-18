--[[
MIT License

Copyright (c) 2019-2021 Marco Lizza

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
  local proto = {}
  proto.__index = proto
  proto.new = function(...)
      local self = setmetatable({}, proto)
      if self.__ctor then
        self:__ctor(...)
      end
      return self
    end
  return proto
end

function Class.borrow(proto, model, criteria)
  for key, value in pairs(model) do
    if type(value) == "function" and key ~= "new" then
      local existing = proto[key]
      if existing and criteria == "extend" then
        proto[key] = function(...)
            existing(...)
            value(...)
          end
      elseif existing and criteria == "chain" then
        proto[key] = function(...)
            value(...)
            existing(...)
          end
      elseif not existing or criteria == "replace" then
        proto[key] = value
      end
    end
  end
end

function Class.dump(t, spaces)
  spaces = spaces or ""
  for k, v in pairs(t) do
    print(spaces .. k .. " " .. type(v) .. " " .. tostring(v))
    if type(v) == "table" then
      if k ~= "__index" and k ~= "__newindex" then
        Class.dump(v, spaces .. " ")
      end
    end
  end
end

return Class
