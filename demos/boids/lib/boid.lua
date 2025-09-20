local Class = require("tofu.core.class")
local Vector2D = require("tofu.util.vector2d")

local Boid = Class.define()

-- local FOV = math.pi / 4 * 3
-- local SIZE = 8

local OBSTACLE_RANGE_MULTIPLIER = 8

local MINIMUM_SPEED = 8
local MAXIMUM_SPEED = 128

local MINIMUM_SPEED_SQUARED = MINIMUM_SPEED * MINIMUM_SPEED
local MAXIMUM_SPEED_SQUARED = MAXIMUM_SPEED * MAXIMUM_SPEED

function Boid:__ctor(position, angle, fov, radius)
  self.color = math.random(2, 16) -- Avoid the BLACK
  self.fov = fov
  self.radius = radius
  self.position = position
  self.velocity = Vector2D.from_polar(angle, MINIMUM_SPEED)
  self.aim = { position = nil, timer = 0, reference = 0 }
  self.radius_squared = radius * radius
end

function Boid:find_flockmates(objects)
  local flockmates = {}
  for _, object in ipairs(objects) do
    if self:is_nearby(object) then
      flockmates[#flockmates + 1] = object
    end
  end
  return flockmates
end

function Boid:is_nearby(object)
  local angle = self.position:angle_to(object.position)
  if math.abs(angle) > self.fov then
    return false
  end

  local distance_squared = self.position:distance_from_squared(object.position)
  -- If the checked object is an obstacle, we detect if far more earlier.
  local range = object.is_obstacle and (self.radius_squared * OBSTACLE_RANGE_MULTIPLIER) or self.radius_squared
  if distance_squared > range then
    return false
  end

  return true
end

function Boid:update(velocity, dt)
  self.velocity:add(velocity)
  local speed_squared = self.velocity:magnitude_squared()
  if speed_squared > 0.0 and speed_squared < MINIMUM_SPEED_SQUARED then
    self.velocity:normalize(MINIMUM_SPEED)
  elseif speed_squared > MAXIMUM_SPEED_SQUARED then
    self.velocity:normalize(MAXIMUM_SPEED)
  end
  self.position:fma(self.velocity, dt)

  local px, py = self.position:unpack()
--  local px, py = self.position:x, self.position:y

  self.aim.timer = self.aim.timer - dt

  local elapsed = self.aim.timer <= 0
  local reached = self.aim.position and self.position:distance_from_squared(self.aim.position) <= self.radius_squared
  local retarget = not self.aim.position and (px < 0 or px >= 512 or py < 0 or py >= 512)

  if elapsed or reached or retarget then
    if self.aim.position and not retarget then
      self.aim.position = nil
    else
      local set_aim = math.random() <= (retarget and 1.0 or 0.333)
      if set_aim then
        local x = math.random(32, 512 - 33)
        local y = math.random(32, 512 - 33)
        self.aim.position = Vector2D.new(x, y)
      end
    end
    self.aim.reference = math.random(5, 15)
    self.aim.timer = self.aim.reference
  end
end

function Boid:draw(canvas)
--  local angle, _ = self.velocity:polar()
  local x, y = self.position:unpack()

  -- local tip = Vector2D.from_polar(angle, SIZE, x, y)
  -- local left_tail = Vector2D.from_polar(angle - FOV, SIZE, x, y)
  -- local right_tail = Vector2D.from_polar(angle + FOV, SIZE, x, y)

  -- canvas:polyline({ tip.x, tip.y, right_tail.x, right_tail.y, left_tail.x, left_tail.y, tip.x, tip.y }, self.color)

  local v = self.velocity:clone()
  v:normalize(8)
  local vx, vy = v:unpack()

  local x0, y0 = x, y
  local x1, y1 = x0 + vx, y0 + vy
  canvas:line(x0, y0, x1, y1, self.color)
end

return Boid
