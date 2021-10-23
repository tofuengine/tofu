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

local Class = require("tofu.core").Class
local System = require("tofu.core").System
local Tweener = require("tofu.generators").Tweener
local Wave = require("tofu.generators").Wave
local Bank = require("tofu.graphics").Bank
local Canvas = require("tofu.graphics").Canvas
local Display = require("tofu.graphics").Display
local Font = require("tofu.graphics").Font
local Palette = require("tofu.graphics").Palette

local EASINGS = {
    "linear",
    "quadratic-in", "quadratic-out", "quadratic-in-out",
    "cubic-in", "cubic-out", "cubic-in-out",
    "quartic-in", "quartic-out", "quartic-in-out",
    "quintic-in", "quintic-out", "quintic-in-out",
    "sine-in", "sine-out", "sine-in-out",
    "circular-in", "circular-out", "circular-in-out",
    "exponential-in", "exponential-out", "exponential-in-out",
    "elastic-in", "elastic-out", "elastic-in-out",
    "back-in", "back-out", "back-in-out",
    "bounce-in", "bounce-out", "bounce-in-out",
  }

local PERIOD = 5.0

local Main = Class.define()

function Main:__ctor()
  Display.palette(Palette.new("pico-8"))

  self.tweeners = {}
  for _, easing in ipairs(EASINGS) do
    table.insert(self.tweeners, Tweener.new(easing))
  end

  local canvas = Canvas.default()
  local width, height = canvas:size()
  local x0, y0 = width * 0.25, height * 0
  self.area = { x = x0, y = y0, width = width * 0.50, height = height * 1 }

  self.bank = Bank.new(Canvas.new("assets/sheet.png", 0), 8, 8)
  self.font = Font.default(0, 15)
  self.wave = Wave.new("triangle", PERIOD)
end

function Main:process()
end

function Main:update(_)
end

function Main:_evaluate(t)
  local y = self.wave(t)
  y = (y + 0.75) / 1.5
  return math.min(math.max(y, 0.0), 1.0)
end

function Main:render(_)
  local canvas = Canvas.default()
  canvas:clear()

  local ratio = self:_evaluate(System.time()) -- The waves have values in the range [-1, +1].

  local area = self.area
  local _, ch = self.bank:size(Bank.NIL)

  local y = area.y
  for index, tweener in ipairs(self.tweeners) do
    local x = area.x + area.width * tweener(ratio)
    canvas:shift(5, 1 + (index % 15))
    self.bank:blit(canvas, x, y, index % 7)
    y = y + ch
  end

  self.font:write(canvas, 0, 0, string.format("FPS: %d", System.fps()))
end

return Main
