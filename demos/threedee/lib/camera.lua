local Class = require("tofu.core").Class

local Camera = Class.define()

function Camera:__ctor(field_of_view, width, height, near, far)
  self.d = 1.0 / math.tan(field_of_view * 0.5)

  self.aspect_ratio = width / height
  self.width = width
  self.height = height
  self.half_width = width * 0.5
  self.half_height = height * 0.5

  self.near = near
  self.far = far
  self.normalized_near = near / far

  self.x = 0.0
  self.y = 250.0
  self.z = 0.0
end

function Camera:set_field_of_view(theta)
  self.d = 1.0 / math.tan(theta * 0.5)
end

function Camera:set_clipping_planes(near, far)
  self.near = near
  self.far = far
  self.normalized_near = near / far
end

function Camera:move(x, y, z)
  self.x = x
  self.y = y
  self.z = z
end

-- function Camera:rotate(angle)
-- end

function Camera:project(x, y, z)
  local cx = x - self.x -- Transform to camera-space.
  local cy = y - self.y
  local cz = z - self.z

  -- TODO: apply rotation here.

  local d_over_dz = self.d / cz -- Transform to normalized projection plane
  local px = cx * d_over_dz / self.aspect_ratio
  local py = cy * d_over_dz
  local pz = cz / self.far

  if px < -1 or px > 1 or py < -1 or py > 1 or pz < self.normalized_near or pz > 1 then
    return px, py, pz, nil, nil
  end

  -- Remap from `[-1, +1]` to `[0, width)` and `[0, height)`. The formula is
  --
  --   ((x + 1) / 2) * width - 1
  --   ((-y + 1) / 2) * height - 1
  --
  -- which can be simplified by pre-calculating the half width/height. Also note that the
  -- `y` component is mirrored as the display origin is at the top-left corner.
  local sx = (1.0 + px) * self.half_width - 1
  local sy = (1.0 - py) * self.half_height - 1

  return px, py, pz, sx, sy
end

return Camera
