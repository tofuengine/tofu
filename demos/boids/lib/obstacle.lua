local Class = require("tofu.core.class")
local Vector2D = require("tofu.util.vector2d")

local Obstacle = Class.define()

function Obstacle:__ctor(position, angle)
  self.is_obstacle = true
  self.color = 1
  self.position = position
  self.velocity = Vector2D.from_polar(angle, 0.0)
end

function Obstacle:find_flockmates(_)
  return {}
end

function Obstacle:is_nearby(_)
  return false
end

function Obstacle:update(_, _)
end

function Obstacle:draw(canvas)
  local position = self.position

  canvas:circle('fill', position.x, position.y, 3, self.color)
end

return Obstacle
