--[[
MIT License

Copyright (c) 2019-2022 Marco Lizza

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

local Vector = Class.define()

-- TODO: optimize by using `{ x, y }` over `{ x = x, y = y }`.

function Vector:__ctor(...)
  local args = { ... }
  if #args == 0 then
    self.x, self.y = 0, 0
  elseif #args == 1 then
    local v = args[1]
    self.x, self.y = v.x, v.y
  elseif #args == 2 then
    self.x, self.y = args[1], args[2]
  else
    error("invalid arguments for Vector")
  end
end

function Vector:__eq(v)
  return self == v or (self.x == v.x and self.y == v.y)
end

function Vector:__tostring()
	return string.format("<%.5f, %.5f>", self.x, self.y)
end

function Vector.from_points(a, b)
  return Vector.new(b.x - a.x, b.y - a.y)
end

function Vector.from_polar(a, l, ox, oy)
  return Vector.new(math.cos(a) * l + (ox and ox or 0), math.sin(a) * l + (oy and oy or 0))
end

function Vector:to_polar()
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
  local v = Vector.from_points(p0, p1)
  local t0 = v:perp_dot(v1) / det -- ratio for the first ray
  local t1 = v:perp_dot(v0) / det -- ratio for the second ray
  return t0, t1
end

function Vector:unpack()
  return self.x, self.y
end

function Vector:pack()
  return { self.x, self.y }
end

local EPSILON <const> = 1.19209290e-7

function Vector:is_almost_zero()
  return math.abs(self.x) <= EPSILON and math.abs(self.y) <= EPSILON
end

function Vector:is_zero()
  return self.x == 0 and self.y == 0
end

function Vector:is_almost_equal(v)
  return math.abs(self.x - v.x) <= EPSILON and math.abs(self.y - v.y) <= EPSILON
end

function Vector:is_equal(v)
  return self.x == v.x and self.y == v.y
end

function Vector:assign(v)
  self.x, self.y = v.x, v.y
end

function Vector:add(v)
  self.x, self.y = self.x + v.x, self.y + v.y
end

function Vector:sub(v)
  self.x, self.y = self.x - v.x, self.y - v.y
end

function Vector:mul(v)
  self.x, self.y = self.x * v.x, self.y * v.y
end

function Vector:div(v)
  self.x, self.y = self.x / v.x, self.y / v.y
end

function Vector:scale(s)
  self.x, self.y = self.x * s, self.y * s
end

function Vector:fma(v, t) -- Fused multiply-add
  self.x, self.y = self.x + v.x * t, self.y + v.y * t
end

function Vector:lerp(v, t)
  self.x, self.y = self.x * (1.0 - t) + v.x * t, self.y * (1.0 - t) + v.y * t
end

-- | cos(a)  -sin(a) | | x |   | x' |
-- |                 | |   | = |    |
-- | sin(a)   cos(a) | | y |   | y' |
function Vector:rotate(angle)
  local cos, sin = math.cos(angle), math.sin(angle)
  local x, y = self.x, self.y
  self.x, self.y = cos * x - sin * y, sin * x + cos * y
end

function Vector:rotate_around(angle, pivot)
  self:sub(pivot)
  self:rotate(angle)
  self:add(pivot)
end

function Vector:negate()
  self.x, self.y = -self.x, -self.y
end

-- COUNTER-CLOCKWISE perpendicular vector (`perp` operator).
function Vector:rotate90ccw()
  self.x, self.y = -self.y, self.x
end

-- CLOCKWISE perpendicular vector.
function Vector:rotate90cw()
  self.x, self.y = self.y, -self.x
end

Vector.rotate180 = Vector.negate

-- a dot b
-- ------- b
-- b dot b
--
-- https://en.wikipedia.org/wiki/Vector_projection
function Vector:project(v)
  local s = self:dot(v) / v:dot(v)
  self:scale(s)
end

--       a dot b
-- a - 2 ------- b
--       b dot b
--
-- https://math.stackexchange.com/questions/2239169/reflecting-a-vector-over-another-line
function Vector:mirror(v)
  local s = 2 * self:dot(v) / v:dot(v)
  self.x, self.y = self.x - s * v.x, self.y - s * v.y
end

function Vector:dot(v)
  return self.x * v.x + self.y * v.y
end

Vector.perp = Vector.rotate90ccw

-- Area of the parallelogram described by the vector, i.e. the DETERMINAND of
-- the matrix with the vectors as columns (or rows).
--
-- It is also (if scaled) the sine of the angle between the vectors. That means
-- that if NEGATIVE the second vector is CLOCKWISE from the first one, if
-- POSITIVE the second vector is COUNTER-CLOCKWISE from the first one.
--
-- It is also called "perp-dot", that is the dot product of the perpendicular
-- vector with another vector (i.e. `a:perp():dot(b)`)
--
-- NOTE: when on a 2D display, since the `y` component inverts it sign, also
--       the rule inverts! That is if NEGATIVE then is COUNTER-CLOCKWISE.
--
-- https://en.wikipedia.org/wiki/Exterior_algebra
-- http://geomalgorithms.com/vector_products.html#2D-Perp-Product
function Vector:perp_dot(v)
  return self.x * v.y - self.y * v.x
end

function Vector:magnitude_squared()
  local x, y = self.x, self.y
  return x * x + y * y
end

function Vector:magnitude()
  return self:magnitude_squared() ^ 0.5
end

function Vector:distance_from_squared(v)
  local dx, dy = self.x - v.x, self.y - v.y
  return dx * dx + dy * dy
end

function Vector:distance_from(v)
  return self:distance_from_squared(v) ^ 0.5
end

function Vector:normalize(l)
  local magnitude = self:magnitude_squared()
  if magnitude == 0 then
    return 0
  end
  self:scale((l or 1) * (magnitude ^ -0.5))
  return magnitude
end

-- Normalize to the given `l` length only when greater than it.
function Vector:trim(l)
  local s = l * l / self:magnitude_squared()
  if s >= 1 then
    return
  end
  self:scale(s ^ 0.5)
end

function Vector:trim_if_not_zero(l)
  if self:is_zero() then
    return
  end
  self:trim(l)
end

function Vector:angle_to(v)
  if v then
    return math.atan(v.y - self.y, v.x - self.x)
  end
  return math.atan(self.y, self.x)
end

function Vector:angle_between(v)
  return math.atan(self.y, self.x) - math.atan(v.y, v.x)
end

Vector.ZERO = Vector.new(0, 0)

return Vector