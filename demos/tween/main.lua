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

local Math = require("tofu.core").Math
local System = require("tofu.core").System
local Bank = require("tofu.graphics").Bank
local Canvas = require("tofu.graphics").Canvas
local Display = require("tofu.graphics").Display
local Font = require("tofu.graphics").Font
local Class = require("tofu.util").Class
local Easing = require("tofu.util").Easing

local EASINGS = {
    "linear", "linear", "linear",
    "quadratic_in", "quadratic_out", "quadratic_in_out",
    "cubic_in", "cubic_out", "cubic_in_out",
    "sine_in", "sine_out", "sine_in_out",
    "circular_in", "circular_out", "circular_in_out",
    "exponential_in", "exponential_out", "exponential_in_out",
    "elastic_in", "elastic_out", "elastic_in_out",
    "back_in", "back_out", "back_in_out",
    "bounce_in", "bounce_out", "bounce_in_out",
  }

local PERIOD = 5.0

local Main = Class.define()

function Main:__ctor()
  Display.palette("pico-8")

  self.tweeners = {}
  for _, easing in ipairs(EASINGS) do
    table.insert(self.tweeners, Easing.tweener(easing))
  end

  self.bank = Bank.new("assets/sheet.png", 8, 8)
  self.font = Font.default(0, 15)
end

function Main:input()
end

function Main:update(_)
end

function Main:render(_)
  local canvas = Canvas.default()
  canvas:clear()

  local ratio = (Math.triangle_wave(PERIOD, System.time()) + 1.0) * 0.5 -- The waves have values in the range [-1, +1].

  local width, _ = canvas:size()
  local _, ch = self.bank:size(-1)

  local y = 0
  for index, tweener in ipairs(self.tweeners) do
    local x = width * tweener(ratio)
    canvas:shift(5, 1 + (index % 15))
    self.bank:blit(index % 7, x, y)
    y = y + ch
  end

  self.font:write(string.format("FPS: %d", System.fps()), 0, 0)
end

return Main
