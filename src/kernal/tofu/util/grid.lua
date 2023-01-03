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

local Grid = {}

--Grid.__index = Grid

-- <width>|<height>|<amount>:<value>|<amount>:<value>|<amount>:<value>
function Grid.parse(content)
  local columns, rows, data = string.match(content, "^(%d+)|(%d+)|(.+)$")
  if not columns or not rows or not data then
    error("grid content is malformed")
  end

  local width = math.tointeger(columns)
  local height = math.tointeger(rows)
  if width <= 0 or height <= 0 then
    error("grid dimensions must be positive")
  end

  local grid = Grid.new(width, height, {})

  local offset = 0
  for amount, value in string.gmatch(data, "(%d+):([+-]?%d+%.?%d*)") do -- Matches any number (not only integer).
    for _ = 1, amount do
      grid:poke(offset, tonumber(value))
      offset = offset + 1
    end
  end

  return grid
end

function Grid:to_string()
  local width, height = self:size()
  local size = width * height

  local content = {}
  table.insert(content, string.format("%d|%d", width, height))

  local value = nil
  local amount = 0
  for offset = 0, size - 1 do
    local v = self:peek(offset)
    if value == v then
      amount = amount + 1
    else
      if amount > 0 then
        table.insert(content, string.format("%d:%f", amount, value))
      end
      value = v
      amount = 1
    end
  end

  if amount > 0 then
    table.insert(content, string.format("%d:%f", amount, value))
  end

  return table.concat(content, "|")
end

return Grid
