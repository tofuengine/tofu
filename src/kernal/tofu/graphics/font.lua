--[[
MIT License

Copyright (c) 2019-2024 Marco Lizza

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

local Bank = require("tofu.graphics.bank")
local Image = require("tofu.graphics.image")

local Font = {}

-- Note: the `__index` metatable reference is set by the module loader.
-- Font.__index = Font

local FONTS <const> = {
    ["5x8"] = { file = "assets/png/spleen-5x8.png", width = 5, height = 8 },
    ["6x12"] = { file = "assets/png/spleen-6x12.png", width = 6, height = 12 },
    ["8x16"] = { file = "assets/png/spleen-8x16.png", width = 8, height = 16 },
    ["12x24"] = { file = "assets/png/spleen-12x24.png", width = 12, height = 24 },
    ["16x32"] = { file = "assets/png/spleen-16x32.png", width = 16, height = 32 },
    ["32x64"] = { file = "assets/png/spleen-32x64.png", width = 32, height = 64 },
  }

function Font.default(...)
  local args = { ... }
  if #args == 2 then -- background_color, foreground_color
    local font = FONTS["5x8"]
    return Font.from_image(font.file, font.width, font.height, args[1], args[2])
  elseif #args == 3 then -- id, background_color, foreground_color
    local font = FONTS[args[1]]
    return Font.from_image(font.file, font.width, font.height, args[2], args[3])
  else
    error("invalid arguments for default font")
  end
end

function Font.from_image(...)
  local args = { ... }
  if #args == 2 then -- file, cells_file
    return Font.new(Bank.new(Image.new(args[1]), args[2]))
  elseif #args == 3 then
    if type(args[2]) == 'string' then -- file, cells_file, alphabet
      return Font.new(Bank.new(Image.new(args[1]), args[2]), args[3])
    else -- file, width, height
      return Font.new(Bank.new(Image.new(args[1]), args[2], args[3]))
    end
  elseif #args == 4 then
    if type(args[2]) == 'string' then -- file, cells_file, background_color, foreground_color
      return Font.new(Bank.new(Image.new(args[1], args[3], args[4]), args[2]))
    else -- file, width, height, alphabet
      return Font.new(Bank.new(Image.new(args[1]), args[2], args[3]), args[4])
    end
  elseif #args == 5 then
    if type(args[2]) == 'string' then -- file, cells_file, alphabet, background_color, foreground_color
      return Font.new(Bank.new(Image.new(args[1], args[4], args[5]), args[2]), args[3])
    else -- file, width, height, background_color, foreground_color
      return Font.new(Bank.new(Image.new(args[1], args[4], args[5]), args[2], args[3]))
    end
  elseif #args == 6 then -- file, width, height, alphabet, background_color, foreground_color
    return Font.new(Bank.new(Image.new(args[1], args[5], args[6]), args[2], args[3]), args[4])
  else
    error("invalid arguments for `from_image` method")
  end
end

function Font:wrap(text, width)
  local lines = {}
  local line = ""
  for c in text:gmatch(".") do
    local lw, _ = self:size(line .. c)
    if lw >= width then
      table.insert(lines, line)
      line = c
    else
      line = line .. c
    end
  end
  if #line > 0 then
    table.insert(lines, line)
  end
  return lines
end

return Font
