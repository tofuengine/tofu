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
local Canvas = require("tofu.graphics").Canvas
local Display = require("tofu.graphics").Display
local Vector = require("tofu.util").Vector

local Sprite = Class.define()

function Sprite:__ctor(bank, from, to, scale)
  self.bank = bank
  self.from = from
  self.to = to
  self.scale = scale

  self.mass = 1.0
  self.inertia = 1.0

  self.position = Vector.new(0, 0)
  self.velocity = Vector.new(0, 0)
  self.acceleration = 0
  self.angular_velocity = 0
  self.angle = 0
end

function Sprite:move(x, y)
  self.position = Vector.new(x, y)
end

function Sprite:rotate(torque)
  -- When updating the angular velocity we should also take into account
  -- that steering is easier at higher speeds.
  self.angular_velocity = self.angular_velocity + torque / self.inertia
end

function Sprite:accelerate(force)
  self.acceleration = self.acceleration + force / self.mass
end

function Sprite:update(delta_time)
  self.angle = self.angle + self.angular_velocity * delta_time
  self.angular_velocity = self.angular_velocity * 0.90

  self.velocity:add(Vector.from_polar(self.angle, self.acceleration * delta_time))
  self.acceleration = self.acceleration * 0.90

  self.position:add(self.velocity:scale(delta_time))
  self.velocity:scale(0.95)
end

function Sprite:render()
  local x, y = self.position.x, self.position.y
  for id = self.from, self.to do
    local i = (id - self.from) * self.scale
    for j = i, i + self.scale - 1 do
      local rotation = Math.angle_to_rotation(self.angle - math.pi * 0.5)
      self.bank:blit(id, x, y - j, rotation, self.scale)
    end
  end

--  var direction = Vector2D.fromPolar(_angle, 48)
--  Canvas.line(x, y, x + direction.x, y + direction.y, 32)
  local canvas = Canvas.default()
  local color = Display.color_to_index(0xFFFF4444)
  canvas:line(x, y, x + self.velocity.x, y + self.velocity.y, color)
end

return Sprite
