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
  self.depth = far - near

  self.position = { 0.0, 0.0, 0.0 }
end

function Camera:set_field_of_view(theta)
  self.d = 1.0 / math.tan(theta * 0.5)
end

function Camera:set_clipping_planes(near, far)
  self.near = near
  self.far = far
  self.depth = far - near
end

function Camera:move(x, y, z)
  self.position = { x, y, z }
end

-- function Camera:rotate(angle)
-- end

local function _is_visible(x, y, z)
  return (x >= -1 and x <= 1) and (y >= -1 and y <= 1) and (z >= 0 and z <= 1)
end

function Camera:project(x, y, z)
  local ox, oy, oz = table.unpack(self.position)

  local dx = x - ox -- Transform to camera-space.
  local dy = y - oy
  local dz = z - oz

  -- TODO: apply rotation here.

  local d_over_dz = self.d / dz -- Transform to normalized projection plane
  local cx = dx * d_over_dz / self.aspect_ratio
  local cy = dy * d_over_dz
  local cz = (dz - self.near) / self.depth

  if not _is_visible(cx, cy, cz) then
    return { cx, cy, cz }, nil
  end

  -- Remap from `[-1, +1]` to `[0, width)` and `[0, height)`. The formula is
  --
  --   ((x + 1) / 2) * width - 1
  --   ((-y + 1) / 2) * height - 1
  --
  -- which can be simplified by pre-calculating the half width/height. Also note that the
  -- `y` component is mirrored as the display origin is at the top-left corner.
  local sx = (1.0 + cx) * self.half_width - 1
  local sy = (1.0 - cy) * self.half_height - 1

  return { cx, cy, cz }, { sx, sy }
end

return Camera
