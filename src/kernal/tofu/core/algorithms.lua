--[[
MIT License

Copyright (c) 2019-2023 Marco Lizza

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

local Algorithms = {}

function Algorithms.bresenham(x0, y0, x1, y1, _, callback)
  local delta_x <const> = x1 - x0
  local delta_y <const> = y1 - y0

  local delta <const> = math.abs(delta_x) >= math.abs(delta_y)
    and math.abs(delta_x)
    or math.abs(delta_y) -- Move along the longest delta

  local step_x <const> = delta_x / delta
  local step_y <const> = delta_y / delta

  local x = x0
  local y = y0

  callback(x0, y0)
  for _ = 1, delta - 1 do
    x = x + step_x
    y = y + step_y

    callback(x, y)
  end
  callback(x1, y1)
end

return Algorithms
