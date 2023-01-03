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

local Image = require("tofu.graphics.image")

local Canvas = {}

local _default = nil

function Canvas.default()
  if not _default then
    local image = Image.new() -- Get a reference to the VRAM as an `Image`.
    _default = Canvas.new(image)
  end
  return _default
end

function Canvas:square(mode, x, y, size, index)
  self:rectangle(mode, x, y, size, size, index)
end

-- Only `font`, `x`, `y`, and `text` are required. All the other arguments are optional.
--
-- From the [reference manual](https://www.lua.org/pil/5.1.html)
-- << [...] A function call that is not the last element in the list always produces one
-- result [...] When a function call is the last (or the only) argument to another call,
-- all results from the first call go as arguments. >>
function Canvas:write(x, y, font, text, h_align, v_align, scale_x, scale_y)
  local width, height = font:size(text, scale_x or 1.0, scale_y or scale_x or 1.0)

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

  if scale_y then
    return self:text(x - dx, y - dy, font, text, scale_x, scale_y)
  elseif scale_x then
    return self:text(x - dx, y - dy, font, text, scale_x)
  else
    return self:text(x - dx, y - dy, font, text)
  end
end

return Canvas
