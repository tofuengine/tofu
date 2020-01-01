--[[
  Copyright (c) 2019-2020 by Marco Lizza (marco.lizza@gmail.com)

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

function Math.lerp(a, b, r)
  -- More precise than `a + (b - a) * r`.
  return a * (1.0 - r) + b * r
end

function Math.sine_wave(period, t)
  local t_over_p = t / period
  return math.sin(t_over_p * 2 * math.pi)
end

function Math.square_wave(period, t)
  local t_over_p = t / period
  return 2.0 * (2.0 * math.floor(t_over_p) - math.floor(2 * t_over_p)) + 1.0
end

function Math.triangle_wave(period, t)
  local t_over_p = t / period
  return 2.0 * math.abs(2.0 * (t_over_p - math.floor(t_over_p + 0.5))) - 1.0
end

function Math.sawtooth_wave(period, t)
  local t_over_p = t / period
  return 2.0 * (t_over_p - math.floor(0.5 + t_over_p))
end

return Math
