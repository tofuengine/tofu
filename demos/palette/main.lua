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

local Class = require("tofu.core.class")
local System = require("tofu.core.system")
local Input = require("tofu.events.input")
local Noise = require("tofu.generators.noise")
local Wave = require("tofu.generators.wave")
local Bank = require("tofu.graphics.bank")
local Canvas = require("tofu.graphics.canvas")
local Display = require("tofu.graphics.display")
local Palette = require("tofu.graphics.palette")
local Font = require("tofu.graphics.font")

local Main = Class.define()

local AMOUNT <const> = 16
local PALETTES <const> = { "pico-8", "arne-16", "dawnbringer-16", "c64", "cga" }

--384 x 224 pixels
local BYTES <const> = "iVBORw0KGgoAAAANSUhEUgAAAQAAAAEACAYAAABccqhmAAAC4klEQVR42u3ZTQrCMBCA0dzQpUvvf4qKC0Exk5k0DW7e4lG"
  .. "1tEjLfPSntdaOl8f9dvQ+976Pfou2jZbZ9tX173XZPoAvv8PzOUSVz9kAViKQDXC2/+j/AwPR8FRVAjI7/L249AI0EyhgIQ"
  .. "DRgFW3i5a7OcmwEIDqLcK/I+AKADY8A7jiHvxsBGZuAaJbCicYkgBUBzx72n9m+K94C+DyHxYDkA129ZXf6JXhrrcA1YAAg"
  .. "wAAAgAIACAAgAAAAgAIACAAgAAAAgAIACAAgAAAAgAIACAAgAAAAgAIACAAgAAAAgAIACAAgAAAAgAIACAAgAAAAgAIACAA"
  .. "gAAAAgAIACAAIACAAAACAAgAIACAAAACAAgAIACAAAACAAgAIACAAAACAAgAIACAAAACAAgAIACAAAACAAgAIACAAAACAAg"
  .. "AIACAAAACAAgAIACAAAACAAgA4ACAAAACAAgAIACAAAACAAgAIACAAAACAAgAIACAAAACAAgAIACAAAACAAgAIACAAAACAA"
  .. "gAIACAAAACAAgAIACAAAACAAgAIACAAAACAAgAIACAAwACAAgAIACAAAACAAgAIACAAAACAAgAIACAAAACAAgAIACAAAACA"
  .. "AgAIACAAAACAAgAIACAAAACAAgAIACAAAACAAgAIACAAAACAAgAIACAAAAOAggAIACAAAACAAgAIACAAAACAAgAIACAAAAC"
  .. "AAgAIACAAAACAAgAIACAAAACAAgAIACAAAACAAgAIACAAAACAAgAIACAAAACAAgAIACAAAACAALgIIAAAAIACAAgAIAAAAI"
  .. "ACAAgAIAAAAIACAAgAIAAAAIACAAgAIAAAAIACAAgAIAAAAIACAAgAIAAAAIACAAgAIAAAAIACAAgAIAAAAIACAAgACAAgA"
  .. "AAAgAIACAAgAAAAgAIACAAgAAAAgAIACAAgAAAAgAIACAAgAAAAgAIACAAgAAAAgAIACAAgAAAAgAIAHDaEx9nKGWcpKyrA"
  .. "AAAAElFTkSuQmCC"

function Main:__ctor()
  local canvas = Canvas.default()
  local width, height = canvas:size()

  self.bank = Bank.new(Canvas.new(BYTES, 0, 5), 8, 8)
  self.font = Font.default(0, 15)
  self.wave = Wave.new("triangle", 10.0, 128.0)
  self.x_size = width / AMOUNT
  self.y_size = height / AMOUNT
  self.palette = 1
  self.scale_x = 1.0
  self.scale_y = -1.0
  self.x, self.y = canvas:center()
  self.mode = 0
  self.clipping = false

  self.noise = Noise.new("simplex", 1234, 0.02)

  Display.palette(Palette.default("pico-8"))

  Input.auto_repeat("y", 0.5)
end

function Main:process()
  if Input.is_pressed("down") then
    self.scale_y = 1.0
    self.y = self.y + 1
  elseif Input.is_pressed("up") then
    self.scale_y = -1.0
    self.y = self.y - 1
  elseif Input.is_pressed("right") then
    self.scale_x = 1.0
    self.x = self.x + 1
  elseif Input.is_pressed("left") then
    self.scale_x = -1.0
    self.x = self.x - 1
  elseif Input.is_pressed("y") then
    self.mode = (self.mode + 1) % 10
  elseif Input.is_pressed("x") then
    self.clipping = not self.clipping
    local canvas = Canvas.default()
    if self.clipping then
      canvas:clipping(32, 32, 64, 64)
    else
      canvas:clipping()
    end
  end
end

local function _sort_palette(palette) -- Sort the palette by increasing luminance.
  local colors = palette:colors()
  table.sort(colors, function(a, b)
      local av = (a[1] + a[2] + a[3]) / 3
      local bv = (b[1] + b[2] + b[3]) / 3
      return av < bv
    end);
  return Palette.new(colors)
end

function Main:update(_)
  local index = (math.tointeger(System.time() * 0.2) % #PALETTES) + 1
  if self.palette ~= index then
    self.palette = index
    local palette = _sort_palette(Palette.default(PALETTES[index]))
    Display.palette(_sort_palette(palette))
  end
end

function Main:render(_)
  local canvas = Canvas.default()
  local width, height = canvas:size()
  canvas:clear()

  local time = System.time()

  if self.mode == 0 then
    for i = 0, AMOUNT - 1 do
      local x = self.x_size * i
      for j = 0, AMOUNT - 1 do
        local index = (i + j) % 7
        local color = (i + j) % AMOUNT
        local y = (height - 8) * (math.sin(time * 1.5 + i * 0.250 + j * 0.125) + 1) * 0.5
        canvas:shift(5, color)
        self.bank:blit(canvas, x, y, index)
      end
    end
  elseif self.mode == 1 then
    for i = 0, AMOUNT - 1 do
      local x = self.x_size * i
      for j = 0, AMOUNT - 1 do
        local index = (i + j) % 7
        local color = (i + j) % AMOUNT
        local y = self.y_size * j
        canvas:shift(5, color)
        self.bank:tile(canvas, x, y, index, 0, math.tointeger(time * 4))
      end
    end
  elseif self.mode == 2 then
    for i = 0, AMOUNT - 1 do
      local x = self.x_size * i
      for j = 0, AMOUNT - 1 do
        local index = (i + j) % 7
        local color = (i + j) % AMOUNT
        local y = self.y_size * j
        canvas:shift(5, color)
        self.bank:tile(canvas, x, y, index, math.tointeger(time * 4), 0)
      end
    end
  elseif self.mode == 3 then
    self.bank:tile(canvas, 0, 0, 5, 0, math.tointeger(time * 4), 4, -4)
  elseif self.mode == 4 then
    local scale = (math.cos(time) + 1) * 3 * 0 + 5
    local rotation = math.tointeger(math.sin(time * 0.5) * 512)

    self.bank:blit(canvas, width / 2, height / 2, 0, scale, scale, rotation)
    self.font:write(canvas, width, height, string.format("scale %d, rotation %d", scale, rotation), "right", "bottom")
  elseif self.mode == 5 then
    self.bank:blit(canvas, width / 2, height / 2, 0, 10, 10, 256 * 1)
  elseif self.mode == 6 then
    self.bank:blit(canvas, width / 2, height / 2, 0, 10, 10, 128 * 1)
  elseif self.mode == 7 then
    local x = (width + 16) * (math.cos(time * 0.75) + 1) * 0.5 - 8
    local y = (height + 16) * (math.sin(time * 0.25) + 1) * 0.5 - 8
    self.bank:blit(canvas, x - 4, y - 4, 0)
  elseif self.mode == 8 then
    self.bank:blit(canvas, self.x - 32, self.y - 32, 1, self.scale_x * 8.0, self.scale_y * 8.0)
  elseif self.mode == 9 then
    local noise = self.noise
    canvas:scan(function(x, y, _)
        local v = noise:generate(x, y, time * 5.0)
        return math.tointeger(v * 15)
      end)
  end

  self.font:write(canvas, 0, 0, string.format("FPS: %d", System.fps()), 1.5)
  self.font:write(canvas, width, 0, string.format("mode: %d", self.mode), "right")
end

return Main
