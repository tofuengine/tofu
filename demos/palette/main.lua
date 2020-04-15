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

local Class = require("tofu.core").Class
local Math = require("tofu.core").Math
local System = require("tofu.core").System
local Input = require("tofu.events").Input
local Bank = require("tofu.graphics").Bank
local Canvas = require("tofu.graphics").Canvas
local Display = require("tofu.graphics").Display
local Font = require("tofu.graphics").Font
local Speakers = require("tofu.sound").Speakers

local Main = Class.define()

local AMOUNT = 16
local PALETTES = { "pico-8", "arne-16", "dawnbringer-16", "c64", "cga" }

--384 x 224 pixels

function Main:__ctor()
  Display.palette("pico-8")

  Input.auto_repeat("y", 0.5)

  local canvas = Canvas.default()
  local width, height = canvas:size()

  self.bank = Bank.new("assets/sheet.png", 8, 8)
  self.font = Font.default(0, 15)
  self.wave = Math.wave("triangle", 10.0, 128.0)
  self.x_size = width / AMOUNT
  self.y_size = height / AMOUNT
  self.palette = 1
  self.scale_x = 1.0
  self.scale_y = -1.0
  self.x, self.y = canvas:center()
  self.mode = 0
  self.clipping = false
end

function Main:input()
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

function Main:update(_)
  local index = (math.tointeger(System.time() * 0.2) % #PALETTES) + 1
  if self.palette ~= index then
    self.palette = index
    Display.palette(PALETTES[index])
  end

  Speakers.balance(math.sin(System.time() * 0.25))
  Speakers.volume((math.sin(System.time() * 0.5) + 1.0) * 0.5)
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
        self.bank:blit(index, x, y)
      end
    end
  elseif self.mode == 1 then
    local scale = (math.cos(time) + 1) * 3 * 0 + 5
--    local rotation = math.sin(time * 0.5) * 512
    local rotation = self.wave(time)
    self.bank:blit(0, width / 2, height / 2, rotation, scale)
  elseif self.mode == 2 then
    self.bank:blit(0, width / 2, height / 2, 256 * 1, 10)
  elseif self.mode == 3 then
    self.bank:blit(0, width / 2, height / 2, 128 * 1, 10)
  elseif self.mode == 4 then
    local x = (width + 16) * (math.cos(time * 0.75) + 1) * 0.5 - 8
    local y = (height + 16) * (math.sin(time * 0.25) + 1) * 0.5 - 8
    self.bank:blit(0, x - 4, y - 4)
  elseif self.mode == 5 then
    self.bank:blit(1, self.x - 32, self.y - 32, 0, self.scale_x * 8.0, self.scale_y * 8.0)
  end

  self.font:write(string.format("FPS: %.1f", System.fps()), 0, 0)
  self.font:write(self.font:align(string.format("mode: %d", self.mode), width, 0, "right"))
end

return Main
