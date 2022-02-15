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

local Bunny = Class.define()

local CELL_ID = 1
local MAX_SPEED = 500
local GRAVITY = 981
local X_DAMPENING = 0.95
local Y_DAMPENING = 0.85

function Bunny:__ctor(bank, width, height)
  local cw, ch = bank:size(CELL_ID)

  self.min_x = 0
  self.min_y = 0
  self.max_x = width - cw
  self.max_y = height - ch

  self.bank = bank
  self.x = (self.max_x - self.min_x) / 2 -- Spawn in the top-center part of the screen.
  self.y = (self.max_y - self.min_y) / 8
  self.vx = (math.random() * MAX_SPEED) - (MAX_SPEED * 0.5)
  self.vy = (math.random() * MAX_SPEED) - (MAX_SPEED * 0.5)
end

function Bunny:update(delta_time)
  self.x = self.x + self.vx * delta_time
  self.y = self.y + self.vy * delta_time

  self.vy = self.vy + GRAVITY * delta_time

  if self.x > self.max_x then
    self.vx = self.vx * X_DAMPENING * -1.0
    self.x = self.max_x
  elseif self.x < self.min_x then
    self.vx = self.vx * X_DAMPENING * -1.0
    self.x = self.min_x
  end

  if self.y > self.max_y then
    self.vy = self.vy * Y_DAMPENING * -1.0
    self.y = self.max_y

    if math.abs(self.vy) <= 400.0 and math.random() <= 0.10 then  -- Jump, occasionally!
      self.vx = self.vx - ((math.random() * 100.0) - 50.0)
      self.vy = self.vy - ((math.random() * 500.0) + 250.0)
    end
  elseif self.y < self.min_y then
    self.vy = 0.0 -- Bump on the ceiling!
    self.y = self.min_y
  end
end

function Bunny:render(canvas)
  canvas:sprite(self.x, self.y, self.bank, CELL_ID)
end

return Bunny