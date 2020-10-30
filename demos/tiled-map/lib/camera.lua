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

local Class = require("tofu.core").Class

local function bound(x, y, aabb)
  return math.min(math.max(x, aabb.x0), aabb.x1), math.min(math.max(y, aabb.y0), aabb.y1)
end

local Camera = Class.define()

-- TODO: add camera scaling, useful to draw minimap.
function Camera:__ctor(id, bank, grid, canvas, columns, rows, screen_x, screen_y, anchor_x, anchor_y, scale)
  local cw, ch = bank:size(-1)

  self.id = id
  self.bank = bank
  self.grid = grid
  self.canvas = canvas
  self.screen_x = screen_x or 0
  self.screen_y = screen_y or 0
  self.columns = columns
  self.rows = rows
  self.map_width, self.map_height = cw * columns, ch * rows

  self:scale_by(scale or 1.0)
  self:center_at(anchor_x or 0.5, anchor_y or 0.5)
end

function Camera:scale_by(scale)
  self.scale = scale
  self.screen_width = math.tointeger(self.map_width * scale)
  self.screen_height = math.tointeger(self.map_height * scale)
end

function Camera:center_at(anchor_x, anchor_y)
  local cw, ch = self.bank:size(-1)
  local gw, gh = self.grid:size()

  self.anchor_x = anchor_x
  self.anchor_y = anchor_y
  self.center_x = math.tointeger(anchor_x * self.map_width) -- Always an integer offset
  self.center_y = math.tointeger(anchor_y * self.map_height)
  self.aabb = {
      x0 = self.center_x,
      y0 = self.center_y,
      x1 = gw * cw - (self.map_width - self.center_x) - 1,
      y1 = gh * ch - (self.map_height - self.center_y) - 1
    }
  self:move_to(self.x or 0, self.y or 0)
end

function Camera:move_to(x, y)
  self.x, self.y = bound(x, y, self.aabb)

  local map_x = math.tointeger(self.x) - self.center_x
  local map_y = math.tointeger(self.y) - self.center_y

  if self.map_x == map_x and self.map_y == map_y then
    return
  end
  self.map_x, self.map_y = map_x, map_y -- Track offsetted map position to track *real* changes.

  local scale = self.scale
  local cw, ch = self.bank:size(-1)
  local start_column = math.tointeger(map_x / cw)
  local start_row = math.tointeger(map_y / ch)
  local column_offset = -math.tointeger((map_x % cw) * scale) -- In screen coordinates.
  local row_offset = -math.tointeger((map_y % ch) * scale)

  if self.start_column ~= start_column or self.start_row ~= start_row then
    self.start_column = start_column
    self.start_row = start_row
    self:prepare_()
  end
  self.column_offset = column_offset
  self.row_offset = row_offset
end

function Camera:to_screen(x, y)
  local scale = self.scale
  return (x - self.x + self.center_x) * scale + self.screen_x, (y - self.y + self.center_y) * scale + self.screen_y
end

function Camera:to_world(x, y)
  local scale = self.scale
  return (x - self.screen_x) / scale + self.center_x + self.x, (y - self.screen_y) / scale + self.center_y + self.y
end

function Camera:update(_)
  -- Override.
end

function Camera:pre_draw()
  -- Override.
end

function Camera:draw()
  local scale = self.scale

  self.canvas:clipping(self.screen_x, self.screen_y, self.screen_width, self.screen_height)

  local ox, oy = self.screen_x + self.column_offset, self.screen_y + self.row_offset
  for _, v in ipairs(self.batch) do
    local cell_id, cell_x, cell_y = table.unpack(v)
    self.bank:blit(cell_id, cell_x + ox, cell_y + oy, scale, scale)
  end
end

function Camera:post_draw()
  -- Override.
end

function Camera:prepare_()
  local gw, gh = self.grid:size()
  local cw, ch = self.bank:size(-1, self.scale)

  local rows = math.min(gw - self.start_row, self.rows + 1) -- We handle an additional row/column
  local columns = math.min(gh - self.start_column, self.columns + 1) -- for sub-tile scrolling

  local batch = {}
  local y = 0
  local r = self.start_row
  for _ = 1, rows do
    local x = 0
    local c = self.start_column
    for _ = 1, columns do
      local cell_id = self.grid:peek(c, r)
      table.insert(batch, { math.tointeger(cell_id), x, y })
      x = x + cw
      c = c + 1
    end
    y = y + ch
    r = r + 1
  end
  self.batch = batch
end

function Camera:__tostring()
  return string.format("[%s] %.0f %0.f | %.0f %0.f | %0.f %0.f",
      self.id,
      self.x, self.y,
      self.start_column, self.start_row,
      self.column_offset, self.row_offset)
end

return Camera