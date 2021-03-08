--[[
MIT License

Copyright (c) 2019-2021 Marco Lizza

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

local Copperlist = {}

local function _lerp(v0, v1, t)
  -- More numerical stable than the following one.
  -- return (v1 - v0) * t + v0
  -- see: https://en.wikipedia.org/wiki/Linear_interpolation
  return v0 * (1.0 - t) + v1 * t;
end

function Copperlist.gradient(index, markers)
  local copperlist = Copperlist.new()
  local current_y, current_r, current_g, current_b = -1, 0, 0, 0
  for _, marker in ipairs(markers) do
    local wait_y, wait_r, wait_g, wait_b = table.unpack(marker)
    for y = current_y + 1, wait_y do
      local ratio = (y - current_y) / (wait_y - current_y)
      local r, g, b = _lerp(current_r, wait_r, ratio), _lerp(current_g, wait_g, ratio), _lerp(current_b, wait_b, ratio)
      copperlist:wait(0, y)
      copperlist:color(index, r, g, b)
    end
    current_y, current_r, current_g, current_b = wait_y, wait_r, wait_g, wait_b
  end
  return copperlist
end

return Copperlist
