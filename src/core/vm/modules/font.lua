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

local Font = {}

-- Note: the `__index` metatable reference is set by the module loader.
-- Font.__index = Font

local FONTS = {
    ["5x8"] = { file = "5x8", width = 5, height = 8 },
    ["6x12"] = { file = "6x12", width = 6, height = 12 },
    ["8x16"] = { file = "8x16", width = 8, height = 16 },
    ["12x24"] = { file = "12x24", width = 12, height = 24 },
    ["16x32"] = { file = "16x32", width = 16, height = 32 },
    ["32x64"] = { file = "32x64", width = 32, height = 64 },
  }

function Font.default(...)
  local Canvas = require("tofu.graphics").Canvas -- Lazy `require()` to permit initial load.

  local args = { ... }
  if #args == 2 then -- background_color, foreground_color
    local font = FONTS["5x8"]
    return Font.new(Canvas.new(font.file, args[1], args[2]), font.width, font.height)
  elseif #args == 3 then -- id, background_color, foreground_color
    local font = FONTS[args[1]]
    return Font.new(Canvas.new(font.file, args[2], args[3]), font.width, font.height)
  else
    error("invalid arguments for default font")
  end
end

return Font
