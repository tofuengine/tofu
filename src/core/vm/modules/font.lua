--[[
MIT License

Copyright (c) 2019-2020 Marco Lizza

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

local Font = {}

-- Note: the `__index` metatable reference is set by the module loader.
-- Font.__index = Font

function Font.default(canvas, background_color, foreground_color, id)
  return Font.new(canvas, id or "5x8", 0, 0, background_color, foreground_color)
end

-- Only `text`, `x`, and `y` are required. All the other arguments are optional.
--
-- From the [reference manual](https://www.lua.org/pil/5.1.html)
-- << [...] A function call that is not the last element in the list always produces one
-- result [...] When a function call is the last (or the only) argument to another call,
-- all results from the first call go as arguments. >>
function Font:align(text, x, y, h_align, v_align, scale_x, scale_y)
  local width, height = self:size(text, scale_x or 1.0, scale_y or scale_x or 1.0)

  local dx, dy
  if h_align == "center" then
    dx = tonumber(width * 0.5)
  elseif h_align == "right" then
    dx = width
  else
    dx = 0
  end
  if v_align == "middle" then
    dy = tonumber(height * 0.5)
  elseif v_align == "bottom" then
    dy = height
  else
    dy = 0
  end

  -- Return the proper amount of values in order to trigger the correct
  -- `Font.write()` overloaded method.
  if scale_y then
    return text, x - dx, y - dy, scale_x, scale_y
  elseif scale_x then
    return text, x - dx, y - dy, scale_x
  else
    return text, x - dx, y - dy
  end
end

return Font
