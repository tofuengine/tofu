local Class = require("tofu.core").Class

local Camera = Class.define()

function Camera:__ctor(field_of_view, width, height, near, far)
  self.d = 1.0 / math.tan(field_of_view * 0.5)

  self.aspect_ratio = width / height
  self.width = width
  self.height = height
  self.cx = (width - 1) * 0.5 -- As floating-point number to properly center the NPP on the screen.
  self.cy = (height - 1) * 0.5

  self.near = near
  self.far = far
  self.range = far - near
  self.far_side = far / self.d * self.aspect_ratio

  self.x = 0.0
  self.y = 0.0
  self.z = 0.0
end

function Camera:set_field_of_view(theta)
  self.d = 1.0 / math.tan(theta * 0.5)
  self.far_side = self.far / self.d * self.aspect_ratio
end

function Camera:set_clipping_planes(near, far)
  self.near = near
  self.far = far
  self.range = far - near
  self.far_side = far / self.d * self.aspect_ratio -- Max "wide" distance, on the far plane.
end

function Camera:move(x, y, z)
  self.x = x
  self.y = y
  self.z = z
end

function Camera:offset(dx, dy, dz)
  self.x = self.x + dx
  self.y = self.y + dy
  self.z = self.z + dz
end

-- function Camera:rotate(angle)
-- end

function Camera:is_too_far(x, y, z)
  local cx <const> = x - self.x
  local cy <const> = y - self.y
  local cz <const> = z - self.z
  return cx < -self.far_side or cx > self.far_side -- Avoid `math.abs()`, too costly!
    or cy < -self.far_side or cy > self.far_side
    or cz > self.far or cz < self.near
end

function Camera:is_clipped(px, py, pz)
  return px < -1 or px > 1 or py < -1 or py > 1 or pz < 0 or pz > 1
end

function Camera:is_culled(pz)
  return pz < 0 or pz > 1
end

function Camera:project(x, y, z)
  local cx <const> = x - self.x -- Transform to camera-space.
  local cy <const> = y - self.y
  local cz <const> = z - self.z

  -- TODO: apply rotation here.

  -- Transform to normalized projection plane.
  local d_over_cz <const> = self.d / cz -- Scale factor. Division-by-zero has to be excluded with a prior check.
  local px <const> = cx * d_over_cz / self.aspect_ratio -- [-ar, +ar] -> [-1, +1]
  local py <const> = cy * d_over_cz -- [-1, +1]
  local pz <const> = (cz - self.near) / self.range -- [0, 1]

  -- We don't clip here, as it is potentially too costly.

  -- Remap from `[-1, +1]` to `[0, width)` and `[0, height)`. The formula is
  --
  --   ((x + 1) / 2) * width - 1
  --   ((-y + 1) / 2) * height - 1
  --
  -- which can be simplified by pre-calculating the half width/height. Also note that the
  -- `y` component is mirrored as the display origin is at the top-left corner.
  local sx <const> = (1.0 + px) * self.cx
  local sy <const> = (1.0 - py) * self.cy

  return px, py, pz, sx, sy
end

function Camera:screen_to_world(sx, sy, pz)
  local px <const> = (sx / self.cx) - 1.0
  local py <const> = 1.0 - (sy / self.cy)

  if px < -1 or px > 1 or py < -1 or py > 1 then
    return nil, nil, nil
  end

  local cz <const> = pz * self.far

  local d_over_cz <const> = self.d / cz
  local cx <const> = px * self.aspect_ratio / d_over_cz
  local cy <const> = py / d_over_cz

  local x <const> = cx + self.x -- Transform to camera-space.
  local y <const> = cy + self.y
  local z <const> = cz + self.z

  return x, y, z
end

return Camera
