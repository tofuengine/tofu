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
local Math = require("tofu.core").Math
local Tweener = require("tofu.generators").Tweener
local Canvas = require("tofu.graphics").Canvas
local Timer = require("tofu.timers").Timer

local Oscillator = require("libs/oscillator")

local Wave = Class.define()

function Wave:__ctor(_, transparent, _)
  self.stripe = Canvas.new("assets/images/stripes.png", transparent)
  self.tweener = Tweener.new("linear", 5)
  self.oscillator = Oscillator.new("sine", 0.75)
  self.period_current = 0
  self.period_next = 0
  self.timer = Timer.new(self.tweener:duration(), 0, function(_)
    self.period_current = self.period_next
    self.period_next = math.random() + 0.5
  end)
end

function Wave:update(delta_time)
  local period = Math.lerp(self.period_current, self.period_next, self.tweener(self.timer.age))
  self.oscillator:advance(delta_time * period)
end

function Wave:render(canvas)
  local canvas_width, _ = canvas:size()
  local stripe_width, stripe_height = self.stripe:size()
  local span = stripe_height * 0.25
  for i = 0, canvas_width, stripe_width do
    local dy = self.oscillator:value(i * 0.0005) * span
    canvas:blit(i, dy, self.stripe)
  end
end

return Wave
