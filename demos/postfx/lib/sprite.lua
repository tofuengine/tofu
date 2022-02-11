--[[
MIT License

Copyright (c) 2019-2022 Marco Lizza

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

local Sprite = Class.define()

--local MIN_FREQUENCY = 0.25
--local MAX_FREQUENCY = 2.50
--local MAX_ANGLE = math.pi * 2
local PADDING = 16
local STEP = (2.0 * math.pi) / 32.0

function Sprite:__ctor(canvas, bank, index)
  self.bank = bank

  self.angle = STEP * index -- (2.0 * Num.pi) random.float() * MAX_ANGLE
  self.frequency_x = 0.75 --(random.float() * (MAX_FREQUENCY - MIN_FREQUENCY)) + MIN_FREQUENCY
  self.frequency_y = 0.50 --(random.float() * (MAX_FREQUENCY - MIN_FREQUENCY)) + MIN_FREQUENCY
  self.frequency_s = 3.00 --(random.float() * (MAX_FREQUENCY - MIN_FREQUENCY)) + MIN_FREQUENCY

  self.id = math.random(0, 11)

  local width, height = canvas:size()

  self.CENTER_X = width / 2
  self.CENTER_Y = (height - 64) / 2
  self.X_AMPLITUDE = (width / 2) - PADDING
  self.Y_AMPLITUDE = ((height - 64) / 2) - PADDING
end

function Sprite:update(delta_time)
  self.angle = self.angle + delta_time
end

function Sprite:render(canvas)
  local x = self.CENTER_X + math.cos(self.angle * self.frequency_x) * self.X_AMPLITUDE
  local y = self.CENTER_Y + math.sin(self.angle * self.frequency_y) * self.Y_AMPLITUDE

  local s = ((math.sin(self.angle * self.frequency_s) + 1.0) * 0.5) * 1.5 + 0.5
  self.bank:blit(canvas, x, y, self.id, s, s)
end

return Sprite