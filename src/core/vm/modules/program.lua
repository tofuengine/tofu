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

local Math = require("tofu.core").Math

local Program = {}

function Program.gradient(index, markers)
  local program = Program.new()
  local current_y, current_r, current_g, current_b = -1, 0, 0, 0
  for _, marker in ipairs(markers) do
    local wait_y, wait_r, wait_g, wait_b = table.unpack(marker)
    for y = current_y + 1, wait_y do
      local ratio = (y - current_y) / (wait_y - current_y)
      local r, g, b = Math.lerp(current_r, wait_r, ratio),
      Math.lerp(current_g, wait_g, ratio), Math.lerp(current_b, wait_b, ratio)
      program:wait(0, y)
      program:color(index, r, g, b)
    end
    current_y, current_r, current_g, current_b = wait_y, wait_r, wait_g, wait_b
  end
  return program
end

function Program.palette(x, y, palette)
  local program = Program.new()
  program:wait(x, y)
  for index, color in palette do
    local r, g, b = table.unpack(color)
    program:color(index - 1, r, g, b)
  end
  return program
end

-- TODO: add some helper functions to populate the program.

return Program
