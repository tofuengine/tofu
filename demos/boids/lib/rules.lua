local Vector2D = require("tofu.util.vector2d")

local Rules = {}

function Rules.separation(self, flockmates, params)
  local velocity = Vector2D.new()
  for _, object in ipairs(flockmates) do
    if self ~= object and self:is_nearby(object) then
-- TODO: should the "pulse" vector be proportional to the proximity? Nearer is stronger?
--      local distance = self.position:clone():sub(boid.position)
--      velocity:add(distance)
      velocity:add(self.position)
      velocity:sub(object.position)
    end
  end
  velocity:normalize(params.weight)
  return velocity
end

function Rules.alignment(self, flockmates, params)
  local velocity = Vector2D.new()
  for _, object in ipairs(flockmates) do
    if not object.is_obstacle and self ~= object and self:is_nearby(object) then
      velocity:add(object.velocity)
    end
  end
  velocity:normalize(params.weight)
  return velocity
end

function Rules.cohesion(self, flockmates, params)
  local velocity = Vector2D.new()
  -- Compute the centroid of the flockmates (sum of the position divided by
  -- the number of vectors)
  local count = 0
  for _, object in ipairs(flockmates) do
    if not object.is_obstacle and self ~= object and self:is_nearby(object) then
      velocity:add(object.position)
      count = count + 1
    end
  end
  if count > 0 then
    -- Find the center-of-mass and convert to a "direction" vector.
    velocity:smul(1 / count)
    velocity:sub(self.position)
  end
  velocity:normalize(params.weight)
  return velocity
end

function Rules.follow(self, _, params)
  local velocity = Vector2D.new()
  if self.aim and self.aim.position then
    local v = Vector2D.new(self.aim.position)
    v:sub(self.position)
    velocity = v
  end
  velocity:normalize(params.weight)
  return velocity
end

return Rules