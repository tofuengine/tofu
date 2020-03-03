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

local Vector = {}

-- Note: the `__index` metatable reference is set by the module loader.
-- Font.__index = Font

function Vector:clone()  -- !!!
  return Vector.new(self.x, self.y)
end

function Vector.from_polar(a, l, ox, oy)
  return Vector.new(math.cos(a) * l + (ox and ox or 0), math.sin(a) * l + (oy and oy or 0))
end

function Vector:to_polar()  -- !!!
  return math.atan(self.y, self.x), self:magnitude()
end

-- The pairs `{ p0, v0 }` and `{ p1, v1 }` represent the two rays we are going to
-- check for intersection.
--
-- Returns the point of intersection as ratios for the rays (or
-- `nil` if no intersection is detected). If the ratio is negative, then the
-- intersection happened in the "past". If between `0` and `1` it occurs within
-- the velocity vector span. It greater than `1` it will happen in the "future".
--
-- https://www.gamedev.net/forums/topic/647810-intersection-point-of-two-vectors/
-- http://www.tonypa.pri.ee/vectors/tut05.html
function Vector.intersect(p0, v0, p1, v1)
  local det = v0:perp_dot(v1)
  if det == 0.0 then
    return nil, nil
  end
  local v3 = p1:clone():sub(p0)
  local t0 = v3:perp_dot(v1) / det -- ratio for the first ray
  local t1 = v3:perp_dot(v0) / det -- ratio for the second ray
  return t0, t1
end

function Vector:unpack() -- !!!
  return self.x, self.y
end

function Vector:is_zero() -- !!!
  return self.x == 0 and self.y == 0
end

function Vector:is_equal(v) -- !!!
  return self.x == v.x and self.y == v.y
end

function Vector:assign(v) -- !!!
  self.x = v.x
  self.y = v.y
  return self
end

function Vector:add(v) -- !!!
  self.x = self.x + v.x
  self.y = self.y + v.y
  return self
end

function Vector:sub(v) -- !!!
  self.x = self.x - v.x
  self.y = self.y - v.y
  return self
end

function Vector:scale(s) -- !!!
  self.x = self.x * s
  self.y = self.y * s
  return self
end

-- | cos(a)  -sin(a) | | x |   | x' |
-- |                 | |   | = |    |
-- | sin(a)   cos(a) | | y |   | y' |
function Vector:rotate(a) -- !!!
  local cos = math.cos(a)
  local sin = math.sin(a)
  self.x, self.y = cos * self.x - sin * self.y, sin * self.x + cos * self.y
  return self
end

function Vector:negate() -- !!!
  self.x, self.y = -self.x, -self.y
  return self
end

-- COUNTER-CLOCKWISE perpendicular vector (`perp` operator).
function Vector:perpendiculal() -- !!!
  self.x, self.y = -self.y, self.x
  return self
end

-- a dot b
-- ------- b
-- b dot b
--
-- https://en.wikipedia.org/wiki/Vector_projection
function Vector:project(v)
  local s = self:dot(v) / v:dot(v)
  return self:scale(s)
end

--       a dot b
-- a - 2 ------- b
--       b dot b
--
-- https://math.stackexchange.com/questions/2239169/reflecting-a-vector-over-another-line
function Vector:mirror(v) -- !!!
  local s = 2 * self:dot(v) / v:dot(v)
  self.x, self.y = self.x - s * v.x, self.y - s * v.y
  return self
end

function Vector:dot(v) -- !!!
  return (self.x * v.x) + (self.y * v.y)
end

-- Area of the parallelogram described by the vector, i.e. the DETERMINAND of
-- the matrix with the vectors as columns (or rows).
--
-- It is also (if scaled) the sine of the angle between the vectors. That means
-- that if NEGATIVE the second vector is CLOCKWISE from the first one, if
-- POSITIVE the second vector is COUNTER-CLOCKWISE from the first one.
--
-- It is also called "perp-dot", that is the dot product of the perpendicular
-- vector with another vector (i.e. `a:perpendicular():dot(b)`)
--
-- NOTE: when on a 2D display, since the `y` component inverts it sign, also
--       the rule inverts! That is if NEGATIVE then is COUNTER-CLOCKWISE.
--
-- https://en.wikipedia.org/wiki/Exterior_algebra
-- http://geomalgorithms.com/vector_products.html#2D-Perp-Product
function Vector:perp_dot(v)
  return (self.x * v.y) - (self.y * v.x)
end

function Vector:magnitude_squared() -- !!!
  return self:dot(self)
end

function Vector:magnitude()
  return math.sqrt(self:magnitude_squared())
end

function Vector:distance_from_squared(v)
  local dx = self.x - v.x
  local dy = self.y - v.y
  return (dx * dx) + (dy * dy)
end

function Vector:distance_from(v)
  return math.sqrt(self:distance_from_squared(v))
end

function Vector:normalize(l)
  return self:scale((l or 1) / self:magnitude())
end

function Vector:normalize_if_not_zero(l)
  if self:is_zero() then
    return self
  end
  return self:normalize(l)
end

-- Normalize to the give `l` length only when greater than it.
function Vector:trim(l)
  local s = l * l / self:magnitude_squared()
  if s >= 1 then
    return self
  end
  return self:scale(math.sqrt(s))
end

function Vector:trim_if_not_zero(l)
  if self:is_zero() then
    return self
  end
  return self:trim(l)
end

function Vector:angle_to(v)
  if v then
    return math.atan(v.y- self.y, v.x - self.x)
  end
  return math.atan(self.y, self.x)
end

function Vector:angle_between(v)
  return math.atan(self.y, self.x) - math.atan(v.y, v.x)
end

Vector.ZERO = Vector.new(0, 0)

return Vector