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

local Math = {}

-- TODO: add these? https://github.com/MarcoLizza/love-workouts/tree/master/boids/lib/math
-- https://github.com/MarcoLizza/love-workouts/tree/master/anaglyph-3d/lib/math

function Math.lerp(a, b, r)
  if type(a) == 'table' then
    local v = {}
    for i = 1, #a do
      table.insert(v, Math.lerp(a[i], b[i], r))
    end
    return v
  else
    return a * (1 - r) + b * r
    -- More numerical stable than the following one.
    -- return (b - a) * ratio + a
    -- see: https://en.wikipedia.org/wiki/Linear_interpolation
  end
end

function Math.fitting()
  return nil
end

function Math.path()
  return nil
end

return Math
