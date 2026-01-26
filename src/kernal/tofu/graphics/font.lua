--[[
                ___________________  _______________ ___
                \__    ___/\_____  \ \_   _____/    |   \
                  |    |    /   |   \ |    __) |    |   /
                  |    |   /    |    \|     \  |    |  /
                  |____|   \_______  /\___  /  |______/
                                   \/     \/
        ___________ _______    ________.___ _______  ___________
        \_   _____/ \      \  /  _____/|   |\      \ \_   _____/
         |    __)_  /   |   \/   \  ___|   |/   |   \ |    __)_
         |        \/    |    \    \_\  \   /    |    \|        \
        /_______  /\____|__  /\______  /___\____|__  /_______  /
                \/         \/        \/            \/        \

MIT License

Copyright (c) 2019-2026 Marco Lizza

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

local Bank <const> = require("tofu.graphics.bank")
local Image <const> = require("tofu.graphics.image")

local Font <const> = {}

-- Note: the `__index` metatable reference is set by the module loader.
-- Font.__index = Font

local FONTS <const> = {
    ["5x8"] = { file = "assets/img/spleen-5x8.img", width = 5, height = 8 },
    ["6x12"] = { file = "assets/img/spleen-6x12.img", width = 6, height = 12 },
    ["8x16"] = { file = "assets/img/spleen-8x16.img", width = 8, height = 16 },
    ["12x24"] = { file = "assets/img/spleen-12x24.img", width = 12, height = 24 },
    ["16x32"] = { file = "assets/img/spleen-16x32.img", width = 16, height = 32 },
    ["32x64"] = { file = "assets/img/spleen-32x64.img", width = 32, height = 64 },
  }

function Font.default(...)
  local args <const> = { ... }
  if #args == 0 then -- <none>
    local font = FONTS["5x8"]
    return Font.from_image(font.file, font.width, font.height)
  elseif #args == 1 then -- id
    local font = FONTS[args[1]]
    return Font.from_image(font.file, font.width, font.height)
  else
    error("invalid arguments for default font", 2)
  end
end

function Font.from_image(...)
  local args <const> = { ... }
  if #args == 2 then -- file, cells_file
    return Font.new(Bank.new(Image.new(args[1]), args[2]))
  elseif #args == 3 then
    if type(args[2]) == 'string' then -- file, cells_file, alphabet
      return Font.new(Bank.new(Image.new(args[1]), args[2]), args[3])
    else -- file, width, height
      return Font.new(Bank.new(Image.new(args[1]), args[2], args[3]))
    end
  elseif #args == 4 then -- file, width, height, alphabet
    return Font.new(Bank.new(Image.new(args[1]), args[2], args[3]), args[4])
  else
    error("invalid arguments for `from_image` method", 2)
  end
end

function Font:wrap(text, width)
  local lines <const> = {}
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
