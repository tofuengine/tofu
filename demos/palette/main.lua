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
local Noise = require("tofu.generators.noise")
local Wave = require("tofu.generators.wave")
local Bank = require("tofu.graphics.bank")
local Display = require("tofu.graphics.display")
local Palette = require("tofu.graphics.palette")
local Font = require("tofu.graphics.font")
local Controller = require("tofu.input.controller")
local Arrays = require("tofu.util.arrays")

require("preload")

local PALETTE <const> = Palette.default("pico-8")
local PALETTE_FONT <const> = Palette.new({{ 0, 255, 0 }})
local CANVAS <const> = Canvas.default()
local WIDTH <const>, HEIGHT <const> = CANVAS:image():size()
local FONT <const> = Font.default()
local CONTROLLER <const> = Controller.default()
--local DISPLAY <const> = Display.default()
--local AUDIO <const> = Audio.default()

local AMOUNT <const> = 16
local PALETTES <const> = { "pico-8", "arne-16", "dawnbringer-16", "c64", "cga" }

local Main = Class.define()

function Main:__ctor()
  self.bank = Bank.from_image("assets/sheet.img", 8, 8)
  self.wave = Wave.new("triangle", 10.0, 128.0)
  self.x_size = WIDTH / AMOUNT
  self.y_size = HEIGHT / AMOUNT
  self.palette = 1
  self.scale_x = 1.0
  self.scale_y = -1.0
  self.x, self.y = WIDTH / 2, HEIGHT / 2
  self.mode = 0
  self.clipping = false
  self.noise = Noise.new("simplex", 1234, 0.02)

  local state = self.canvas:state()
  state:clipping(0, 0, 32, 32)
  state:remember("clipping")
end

function Main:init()
  -- Remap the sprite-sheet pixels (which is a single color image with
  -- transparency) to use index #1, to avoid continuous shifting.
  self.bank:image():remap({ [0] = 1 })

  Display.palette(PALETTE_FONT, 1) -- Bank 1 is reserved for the font
  self:_change_palette(PALETTE)
end

function Main:deinit()
end

local function _to_luminance(color)
  return color[1] * 0.30 + color[2] * 0.59 + color[3] * 0.11
end

function Main:_change_palette(palette)
  local sorted = palette:colors() -- Sort the palette by increasing luminance value.
  table.sort(sorted, function(a, b)
      return _to_luminance(a) < _to_luminance(b)
    end);

  local shifting = {} -- Match each ordered color ([0, 15]) to the actual palette index, for shifting.
  local colors = palette:colors()
  for from, color in ipairs(sorted) do
    local to = Arrays.index_of(colors, function(value)
        return color[1] == value[1] and color[2] == value[2] and color[3] == value[3]
      end)
    shifting[from - 1] = to - 1
  end
  self.shifting = shifting

  Display.palette(palette)
end

function Main:handle_input()
  if CONTROLLER:is_pressed("down") then
    self.scale_y = 1.0
    self.y = self.y + 1
  elseif CONTROLLER:is_pressed("up") then
    self.scale_y = -1.0
    self.y = self.y - 1
  elseif CONTROLLER:is_pressed("right") then
    self.scale_x = 1.0
    self.x = self.x + 1
  elseif CONTROLLER:is_pressed("left") then
    self.scale_x = -1.0
    self.x = self.x - 1
  elseif CONTROLLER:is_pressed("y") then
    self.mode = (self.mode + 1) % 10
  elseif CONTROLLER:is_pressed("x") then
    self.clipping = not self.clipping
    local state = self.canvas:state()
    if self.clipping then
      state:clipping(32, 32, 64, 64)
    else
      state:clipping()
    end
  end
end

function Main:update(_)
  self:handle_input()

  -- TODO: set up a timer, here
  local index = (math.tointeger(System.time() * 0.2) % #PALETTES) + 1
  if self.palette ~= index then
    self.palette = index
    local palette = Palette.default(PALETTES[index])
    self:_change_palette(palette)
  end
end

function Main:render(_)
  local canvas <const> = CANVAS
  local state <const> = canvas:state()

  canvas:clear(0)

  local time <const> = System.time()

  if self.mode == 0 then
    state:push()
    state:recall("clipping")
    for i = 0, AMOUNT - 1 do
      local x = self.x_size * i
      for j = 0, AMOUNT - 1 do
        local index = (i + j) % 7
        local color = (i + j) % AMOUNT
        local y = (HEIGHT - 8) * (math.sin(time * 1.5 + i * 0.250 + j * 0.125) + 1) * 0.5
        state:shift(1, color)
        canvas:sprite(x, y, self.bank, index)
      end
    end
    state:pop()
  elseif self.mode == 1 then
    state:push()
    for i = 0, AMOUNT - 1 do
      local x = self.x_size * i
      for j = 0, AMOUNT - 1 do
        local index = (i + j) % 7
        local color = (i + j) % AMOUNT
        local y = self.y_size * j
        state:shift(1, color)
        canvas:tile(x, y, self.bank, index, 0, math.tointeger(time * 4))
      end
    end
    state:pop()
  elseif self.mode == 2 then
    state:push()
    for i = 0, AMOUNT - 1 do
      local x = self.x_size * i
      for j = 0, AMOUNT - 1 do
        local index = (i + j) % 7
        local color = (i + j) % AMOUNT
        local y = self.y_size * j
        state:shift(1, color)
        canvas:tile(x, y, self.bank, index, math.tointeger(time * 4), 0)
      end
    end
    state:pop()
  elseif self.mode == 3 then
    canvas:tile(0, 0, self.bank, 0, 0, math.tointeger(time * 4), 4, -4)
  elseif self.mode == 4 then
    local scale = (math.cos(time) + 1) * 3 * 0 + 5
    local rotation = math.tointeger(math.sin(time * 0.5) * 512)

    canvas:sprite(WIDTH / 2, HEIGHT / 2, self.bank, 0, scale, scale, rotation)
    canvas:write(WIDTH, HEIGHT,
      FONT, string.format("scale %d, rotation %d", scale, rotation), "right", "bottom")
  elseif self.mode == 5 then
    canvas:sprite(WIDTH / 2, HEIGHT / 2, self.bank, 0, 10, 10, 256 * 1)
  elseif self.mode == 6 then
    canvas:sprite(WIDTH / 2, HEIGHT / 2, self.bank, 0, 10, 10, 128 * 1)
  elseif self.mode == 7 then
    local x = (WIDTH + 16) * (math.cos(time * 0.75) + 1) * 0.5 - 8
    local y = (HEIGHT + 16) * (math.sin(time * 0.25) + 1) * 0.5 - 8
    canvas:sprite(x - 4, y - 4, self.bank, 0)
  elseif self.mode == 8 then
    canvas:sprite(self.x - 32, self.y - 32, self.bank, 1, self.scale_x * 8.0, self.scale_y * 8.0)
  elseif self.mode == 9 then
    state:push()
      state:shift(self.shifting)
      local noise = self.noise
      canvas:scan(function(x, y, _)
          local v = noise:generate(x, y, time * 5.0)
          return math.tointeger(v * 15)
        end)
    state:pop()
  end

  state:push()
    state:bank(1)
    canvas:write(0, 0, FONT, string.format("%d FPS", System.fps()), 1.5)
    canvas:write(WIDTH, 0, FONT, string.format("mode: %d", self.mode), "right")
  state:pop()
end

return Main
