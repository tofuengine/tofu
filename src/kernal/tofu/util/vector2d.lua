--[[
                ___________________  _______________ ___
                \__    ___/\_____  \ \_   _____/    |   \
                  |    |    /   |   \ |    __) |    |   /
                  |    |   /    |    \|     \  |    |  /
                  |____|   \_______  /\___  /  |______/
                                   \/     \/
        ___________ _______    ________.___ _______  ___________
        \_   _____/ \      \  /  _____/|   |\      \ \_   _____/
         |    __)_  /   |   \/   \  ___|   |/   |   \ |    __)_
         |        \/    |    \    \_\  \   /    |    \|        \
        /_______  /\____|__  /\______  /___\____|__  /_______  /
                \/         \/        \/            \/        \

MIT License

Copyright (c) 2019-2026 Marco Lizza

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

local Vector2D <const> = {}

function Vector2D.from_points(a, b)
  local v <const> = Vector2D.new()
  local ax <const> , ay <const> = a:unpack()
  local bx <const> , by <const> = b:unpack()
  v:assign(bx - ax, by - ay)
  return v
end

function Vector2D.from_polar(a, l, ox, oy)
  local v <const> = Vector2D.new()
  v:cast(a, l, ox, oy)
  return v
end

-- This is a synonym for sake of simplicity.
function Vector2D:clone()
  return Vector2D.new(self)
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
function Vector2D.intersect(p0, v0, p1, v1)
  local det <const> = v0:perp_dot(v1)
  if det == 0.0 then
    return nil, nil
  end
  local v <const> = Vector2D.new(Vector2D.from_points(p0, p1))
  local t0 <const> = v:perp_dot(v1) / det -- ratio for the first ray
  local t1 <const> = v:perp_dot(v0) / det -- ratio for the second ray
  return t0, t1
end

function Vector2D:rotate_around(angle, pivot)
  self:sub(pivot)
  self:rotate(angle)
  self:add(pivot)
end

-- a dot b
-- ------- b
-- b dot b
--
-- https://en.wikipedia.org/wiki/Vector_projection
function Vector2D:project(v)
  local s <const> = self:dot(v) / v:dot(v)
  self:assign(v)
  self:smul(s)
end

--       a dot b
-- a - 2 ------- b
--       b dot b
--
-- https://math.stackexchange.com/questions/2239169/reflecting-a-vector-over-another-line
function Vector2D:mirror(v)
  local s <const> = 2 * self:dot(v) / v:dot(v)
  local vx <const>, vy <const> = v:unpack()
  self:sub(s * vx, s * vy)
end

function Vector2D.__init()
  -- Defining some pseudo-constants.
  Vector2D.ZERO = Vector2D.new(0, 0)
end

return Vector2D