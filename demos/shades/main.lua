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

local Class = require("tofu.core.class")
local System = require("tofu.core.system")
local Canvas = require("tofu.graphics.canvas")
local Display = require("tofu.graphics.display")
local Font = require("tofu.graphics.font")
local Palette = require("tofu.graphics.palette")
local Controller = require("tofu.input.controller")

local PALETTE <const> = Palette.new(2, 3, 2) -- R2G3B2 (7 bits)  -> 128 colors total!
local PALETTE_FONT <const> = Palette.new({{ 0, 255, 0 }})
local FONT <const> = Font.default()
local CANVAS <const> = Canvas.default()
local WIDTH <const>, HEIGHT <const> = CANVAS:image():size()
local STATE <const> = CANVAS:state()
local CONTROLLER <const> = Controller.default()

local STEPS = PALETTE:size()
local LEVELS = STEPS
local TARGET = { 0, 0, 0 }

local function build_table(palette, levels, target)
  local tr, tg, tb = table.unpack(target)
  local lut = {}
  for i = 0, levels - 1 do
    local shifting = {}
    local ratio = i / (levels - 1)
    for j, color in ipairs(palette:colors()) do
      local ar, ag, ab = table.unpack(color)
      local r, g, b = Palette.mix(ar, ag, ab, tr, tg, tb, 1 - ratio)
      shifting[j - 1] = palette:match(r, g, b)
    end
    lut[i] = shifting
  end
  return lut
end

local Main = Class.define()

function Main:__ctor()
  self.lut = build_table(PALETTE, LEVELS, TARGET)
  self.width = WIDTH / STEPS
  self.height = HEIGHT / STEPS
  self.mode = 0
end

function Main:init()
  Display.palette(PALETTE, 0)
  Display.palette(PALETTE_FONT, 1)
end

function Main:deinit()
end

function Main:handle_input()
  if CONTROLLER:is_pressed("y") then
    self.mode = (self.mode + 1) % 10
  end
end

function Main:update(_)
  self:handle_input()
end

function Main:render(_)
  local canvas <const> = CANVAS
  local state <const> = STATE

  canvas:clear(0)

  local image <const> = canvas:image()

  for i = 0, STEPS - 1 do
    local y = self.height * i
    for j = 0, STEPS - 1 do
      local x = self.width * j
      canvas:rectangle("fill", x, y, self.width, self.height, (i + j) % STEPS)
    end
  end

  state:push()
  state:transparent(0, false)
  if self.mode == 0 then
    for i = 0, STEPS - 1 do
      local y = self.height * i
      state:shift(self.lut[i])
      canvas:blit(0, y, image, 0, y, WIDTH, self.height)
    end
  elseif self.mode == 1 then
    for i = 0, STEPS - 1 do
      state:shift(self.lut[i])
      canvas:blit(i, 0, image, i, 0, 1, HEIGHT)
      canvas:blit(WIDTH - 1 - i, 0, image, WIDTH - 1 - i, 0, 1, HEIGHT)
    end
  else
    local t = System.time()
    local index = math.tointeger((math.sin(t * 2.5) + 1) * 0.5 * (STEPS - 1))
    state:shift(self.lut[index])
    canvas:blit(0, 0, image, 0, 0, WIDTH, HEIGHT / 2)
  end
  state:pop()

  state:push()
    state:bank(1)
    canvas:write(0, 0, FONT, string.format("%d FPS", System.fps()))
  state:pop()
end

return Main
