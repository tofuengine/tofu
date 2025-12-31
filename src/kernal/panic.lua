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

Copyright (c) 2019-2025 Marco Lizza

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

local Class <const> = require("tofu.core.class")
local System <const> = require("tofu.core.system")
local Canvas <const> = require("tofu.graphics.canvas")
local Display <const> = require("tofu.graphics.display")
local Palette <const> = require("tofu.graphics.palette")
local Font <const> = require("tofu.graphics.font")

local PALETTE <const> = Palette.new({ { 0, 0, 0 }, { 255, 0, 0 } }) -- Red on black.
local FONT <const> = Font.default()
local CANVAS <const> = Canvas.default()
local WIDTH <const>, _ <const> = CANVAS:image():size()

local TITLE <const> = {
    "Software Failure.",
    "Guru Meditation"
  }

local MARGIN <const> = 4
local STROKE <const> = 2
local SPAN <const> = WIDTH - 2 * MARGIN

local Panic <const> = Class.define()

function Panic:__ctor(message)
  local errors <const> = {}
  for str in string.gmatch(message, "([^\n]+)") do -- Split the error-message into separate lines.
    table.insert(errors, str)
  end

  self.lines = {} -- Pre-calculate lines position and rectangle area.

  local y = MARGIN + STROKE + MARGIN
  for _, text in ipairs(TITLE) do -- Title lines are centered.
    local lw <const>, lh <const> = FONT:size(text)
    table.insert(self.lines, { text = text, x = (WIDTH - lw) * 0.5, y = y })
    y = y + lh
  end
  y = y + MARGIN

  self.rectangle = { -- The rectangle ends here, message follows.
      x = MARGIN,
      y = MARGIN,
      width = WIDTH - MARGIN - MARGIN,
      height = y - MARGIN + (STROKE - 1)
    }

  y = y + STROKE + MARGIN
  for _, line in ipairs(errors) do -- Error lines are left-justified and auto-wrapped.
    local texts <const> = FONT:wrap(line, SPAN)
    for _, text in ipairs(texts) do
      local _ <const>, th <const> = FONT:size(text)
      table.insert(self.lines, { text = text, x = MARGIN, y = y })
      y = y + th
    end
  end
end

function Panic:init()
  Display.palette(PALETTE)
end

function Panic:deinit()
end

function Panic:update(_)
end

function Panic:render(canvas, _)
  local on <const> = (math.floor(System.time()) % 2) == 0

  local rectangle = self.rectangle
  for i = 0, STROKE - 1 do
    canvas:rectangle("line",
      rectangle.x + i, rectangle.y + i,
      rectangle.width - (i * 2), rectangle.height - (i * 2),
      on and 1 or 0)
  end

  for _, line in ipairs(self.lines) do
    canvas:write(line.x, line.y, FONT, line.text)
  end
end

return Panic
