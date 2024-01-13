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
local Controller = require("tofu.input.controller")

local config = require("config")

local SPEED_X <const> = config.player.speed.x or 250.0
local SPEED_Y <const> = config.player.speed.y or 250.0
local SPEED_Z <const> = config.player.speed.z or 500.0

local Player = Class.define()

function Player:__ctor()
  self.x = 0
  self.y = 0
  self.z = 0

  self.dx = 0
  self.dy = 0
  self.dz = 0

  self.pause = false
end

function Player:process()
  local controller <const> = Controller.default()

  local dx = 0
  local dy = 0
  local dz = 1
  if controller:is_down("up") then
    dy = dy + 1
  end
  if controller:is_down("down") then
    dy = dy - 1
  end
  if controller:is_down("left") then
    dx = dx - 1
  end
  if controller:is_down("right") then
    dx = dx + 1
  end
  self:direction(dx, dy, dz)
end

function Player:move(x, y, z)
  self.x = x
  self.y = y
  self.z = z
end

function Player:direction(dx, dy, dz)
  self.dx = dx
  self.dy = dy
  self.dz = dz
end

function Player:update(delta_time)
  self.x = math.min(math.max(self.x + self.dx * SPEED_X * delta_time, -150.0), 150.0)
  self.y = math.min(math.max(self.y + self.dy * SPEED_Y * delta_time, 50.0), 250)
  if not self.pause then
    self.z = self.z + self.dz * SPEED_Z * delta_time
  end
end

function Player:position()
  return self.x, self.y, self.z
end

return Player
