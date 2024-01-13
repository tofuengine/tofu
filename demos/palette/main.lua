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

local Class = require("tofu.core.class")
local System = require("tofu.core.system")
local Controller = require("tofu.input.controller")
local Noise = require("tofu.generators.noise")
local Wave = require("tofu.generators.wave")
local Bank = require("tofu.graphics.bank")
local Canvas = require("tofu.graphics.canvas")
local Display = require("tofu.graphics.display")
local Image = require("tofu.graphics.image")
local Palette = require("tofu.graphics.palette")
local Font = require("tofu.graphics.font")

require("preload")

local Main = Class.define()

local AMOUNT <const> = 16
local PALETTES <const> = { "pico-8", "arne-16", "dawnbringer-16", "c64", "cga" }

function Main:__ctor()
  local canvas = Canvas.default()
  local width, height = canvas:image():size()

  self.bank = Bank.new(Image.new("assets/sheet.png", 0, 5), 8, 8)
  self.font = Font.default(0, 15)
  self.wave = Wave.new("triangle", 10.0, 128.0)
  self.x_size = width / AMOUNT
  self.y_size = height / AMOUNT
  self.palette = 1
  self.scale_x = 1.0
  self.scale_y = -1.0
  self.x, self.y = canvas:image():center()
  self.mode = 0
  self.clipping = false

  self.noise = Noise.new("simplex", 1234, 0.02)

  self:_change_palette(Palette.default("pico-8"))
end

local function _index_of(array, compare, from)
  local length = #array
  for index = from or 1, length do
    local value = array[index]
    if compare(value) then
      return index
    end
  end
  return nil
end

local function _chop(t, len)
  for i = #t, len + 1, -1 do
    t[i] = nil
  end
  return t
end

local function _to_luminance(color)
  return color[1] * 0.30 + color[2] * 0.59 + color[3] * 0.11
end

function Main:_change_palette(palette)
  local count = palette:size()

  local sorted = palette:colors() -- Sort the palette by increasing luminance value.
  table.sort(_chop(sorted, count), function(a, b)
      return _to_luminance(a) < _to_luminance(b)
    end);

  local shifting = {} -- Ma each ordered color ([0, 15]) to the actual palette index, for shifting.
  local colors = _chop(palette:colors(), count)
  for from, color in ipairs(sorted) do
    local to = _index_of(colors, function(value)
        return color[1] == value[1] and color[2] == value[2] and color[3] == value[3]
      end)
    shifting[from] = to
  end
  self.shifting = shifting

  Display.palette(palette)
end

function Main:process()
  local controller = Controller.default()
  if controller:is_pressed("down") then
    self.scale_y = 1.0
    self.y = self.y + 1
  elseif controller:is_pressed("up") then
    self.scale_y = -1.0
    self.y = self.y - 1
  elseif controller:is_pressed("right") then
    self.scale_x = 1.0
    self.x = self.x + 1
  elseif controller:is_pressed("left") then
    self.scale_x = -1.0
    self.x = self.x - 1
  elseif controller:is_pressed("y") then
    print("Y")
    self.mode = (self.mode + 1) % 10
  elseif controller:is_pressed("x") then
    print("X")
    self.clipping = not self.clipping
    local canvas = Canvas.default()
    if self.clipping then
      canvas:clipping(32, 32, 64, 64)
    else
      canvas:clipping()
    end
  end
end

function Main:update(_)
  local index = (math.tointeger(System.time() * 0.2) % #PALETTES) + 1
  if self.palette ~= index then
    self.palette = index
    local palette = Palette.default(PALETTES[index])
    self:_change_palette(palette)
  end
end

function Main:render(_)
  local canvas = Canvas.default()
  local image = canvas:image()
  local width, height = image:size()
  image:clear(0)

  local time = System.time()

  if self.mode == 0 then
    canvas:push()
    for i = 0, AMOUNT - 1 do
      local x = self.x_size * i
      for j = 0, AMOUNT - 1 do
        local index = (i + j) % 7
        local color = (i + j) % AMOUNT
        local y = (height - 8) * (math.sin(time * 1.5 + i * 0.250 + j * 0.125) + 1) * 0.5
        canvas:shift(5, color)
        canvas:sprite(x, y, self.bank, index)
      end
    end
    canvas:pop()
  elseif self.mode == 1 then
    canvas:push()
    for i = 0, AMOUNT - 1 do
      local x = self.x_size * i
      for j = 0, AMOUNT - 1 do
        local index = (i + j) % 7
        local color = (i + j) % AMOUNT
        local y = self.y_size * j
        canvas:shift(5, color)
        canvas:tile(x, y, self.bank, index, 0, math.tointeger(time * 4))
      end
    end
    canvas:pop()
  elseif self.mode == 2 then
    canvas:push()
    for i = 0, AMOUNT - 1 do
      local x = self.x_size * i
      for j = 0, AMOUNT - 1 do
        local index = (i + j) % 7
        local color = (i + j) % AMOUNT
        local y = self.y_size * j
        canvas:shift(5, color)
        canvas:tile(x, y, self.bank, index, math.tointeger(time * 4), 0)
      end
    end
    canvas:pop()
  elseif self.mode == 3 then
    canvas:tile(0, 0, self.bank, 5, 0, math.tointeger(time * 4), 4, -4)
  elseif self.mode == 4 then
    local scale = (math.cos(time) + 1) * 3 * 0 + 5
    local rotation = math.tointeger(math.sin(time * 0.5) * 512)

    canvas:sprite(width / 2, height / 2, self.bank, 0, scale, scale, rotation)
    canvas:write(width, height, self.font, string.format("scale %d, rotation %d", scale, rotation), "right", "bottom")
  elseif self.mode == 5 then
    canvas:sprite(width / 2, height / 2, self.bank, 0, 10, 10, 256 * 1)
  elseif self.mode == 6 then
    canvas:sprite(width / 2, height / 2, self.bank, 0, 10, 10, 128 * 1)
  elseif self.mode == 7 then
    local x = (width + 16) * (math.cos(time * 0.75) + 1) * 0.5 - 8
    local y = (height + 16) * (math.sin(time * 0.25) + 1) * 0.5 - 8
    canvas:sprite(x - 4, y - 4, self.bank, 0)
  elseif self.mode == 8 then
    canvas:sprite(self.x - 32, self.y - 32, self.bank, 1, self.scale_x * 8.0, self.scale_y * 8.0)
  elseif self.mode == 9 then
    canvas:push()
    canvas:transparent({ [0] = false })
    canvas:shift(self.shifting)
    local noise = self.noise
    canvas:scan(function(x, y, _)
        local v = noise:generate(x, y, time * 5.0)
        return math.tointeger(v * 15)
      end)
    canvas:pop()
  end

  canvas:write(0, 0, self.font, string.format("%d FPS", System.fps()), 1.5)
  canvas:write(width, 0, self.font, string.format("mode: %d", self.mode), "right")
end

return Main
