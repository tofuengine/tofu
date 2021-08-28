local Class = require("tofu.core").Class
local Input = require("tofu.events").Input

local SPEED_X <const> = 500.0
local SPEED_Y <const> = 500.0
local SPEED_Z <const> = 250.0

local Player = Class.define()

function Player:__ctor()
  self.x = 0
  self.y = 0
  self.z = 0

  self.dx = 0
  self.dy = 0
  self.dz = 0
end

function Player:process()
  local dx = 0
  local dy = 0
  local dz = 1
  if Input.is_down("up") then
    dy = dy + 1
  end
  if Input.is_down("down") then
    dy = dy - 1
  end
  if Input.is_down("left") then
    dx = dx - 1
  end
  if Input.is_down("right") then
    dx = dx + 1
  end
  self:direction(dx, dy, dz)
end

function Player:direction(dx, dy, dz)
  self.dx = dx
  self.dy = dy
  self.dz = dz
end

function Player:update(delta_time)
  self.x = self.x + self.dx * SPEED_X * delta_time
  self.y = math.max(self.y + self.dy * SPEED_Y * delta_time, 0.0)
  self.z = self.z + self.dz * SPEED_Z * delta_time
end

function Player:position()
  return self.x, self.y, self.z
end

return Player
