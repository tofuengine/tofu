--[[
  Copyright (c) 2019 Marco Lizza (marco.lizza@gmail.com)

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

local Grid = require("tofu.collections").Grid
local Bank = require("tofu.graphics").Bank
local Canvas = require("tofu.graphics").Canvas
local File = require("tofu.io").File
local Class = require("tofu.util").Class

-- https://developer.mozilla.org/en-US/docs/Games/Techniques/Tilemaps/Square_tilemaps_implementation:_Scrolling_maps

local Tilemap = Class.define()

local function bound(x, y, aabb)
  return math.min(math.max(x, aabb.x0), aabb.x1), math.min(math.max(y, aabb.y0), aabb.y1)
end

function Tilemap:__ctor(file, columns, rows, screen_x, screen_y)
  local content = File.read(file)

  local tokens = {}
  for chunk in string.gmatch(content, "[^\n]+") do
    table.insert(tokens, chunk)
  end

  local cells = {}
  for i = 6, #tokens do
    local chunk = tokens[i]
    for cell in string.gmatch(chunk, "[^ ]+") do
      table.insert(cells, tonumber(cell))
    end
  end

  self.bank = Bank.new(tokens[1], tonumber(tokens[2]), tonumber(tokens[3]))
  self.grid = Grid.new(tonumber(tokens[4]), tonumber(tokens[5]), cells)

--  self.tile_width, self.tile_height = self.bank:cell_width(), self.bank:cell_height()
  self.aabb = {
      x0 = 0,
      y0 = 0,
      x1 = self.grid:width() * self.bank:cell_width() - 1,
      y1 = self.grid:height() * self.bank:cell_height() - 1
    }

  self:camera(columns, rows, screen_x, screen_y)
end

function Tilemap:bound(x, y)
  return bound(x, y, self.aabb)
end

function Tilemap:camera(columns, rows, screen_x, screen_y, anchor_x, anchor_y) -- last two arguments are optional
  local camera = {}
  self.camera = camera

  camera.screen_x = screen_x
  camera.screen_y = screen_y
  camera.columns = columns
  camera.rows = rows
  camera.width = columns * self.bank:cell_width()
  camera.height = rows * self.bank:cell_height()

  self:center_at(anchor_x or 0.5, anchor_y or 0.5)
end

function Tilemap:center_at(anchor_x, anchor_y)
  local camera = self.camera
  camera.anchor_x = anchor_x
  camera.anchor_y = anchor_y
  camera.offset_x = math.tointeger(anchor_x * camera.width) -- Always an integer offset
  camera.offset_y = math.tointeger(anchor_y * camera.height)
  camera.aabb = {
      x0 = camera.offset_x,
      y0 = camera.offset_y,
      x1 = self.grid:width() * self.bank:cell_width() - (camera.width - camera.offset_x) - 1,
      y1 = self.grid:height() * self.bank:cell_height() - (camera.height - camera.offset_y) - 1
    }
  self:move_to(camera.x or 0, camera.y or 0)
end

function Tilemap:move_to(x, y)
  local camera = self.camera
  camera.x, camera.y = bound(x, y, camera.aabb)

  local map_x = math.tointeger(camera.x) - camera.offset_x
  local map_y = math.tointeger(camera.y) - camera.offset_y

  if self.map_x == map_x and self.map_y == map_y then
    return
  end
  self.map_x, self.map_y = map_x, map_y -- Track offsetted map position to track *real* changes.

  local cw, ch = self.bank:cell_width(), self.bank:cell_height()
  local start_column = math.tointeger(map_x / cw)
  local start_row = math.tointeger(map_y / ch)
  local column_offset = -(map_x % cw)
  local row_offset = -(map_y % ch)

  if camera.start_column ~= start_column or camera.start_row ~= start_row then
    camera.start_column = start_column
    camera.start_row = start_row
    self.batch = nil -- Starting row/column changed, recreate the batch!
  end
  camera.column_offset = column_offset
  camera.row_offset = row_offset
end

function Tilemap:to_screen(x, y)
  local camera = self.camera
  return x - camera.x + camera.offset_x + camera.screen_x, y - camera.y + camera.offset_y + camera.screen_y
end

function Tilemap:to_world(x, y)
  local camera = self.camera
  return x - camera.screen_x - camera.offset_x + camera.x, y - camera.screen_y - camera.offset_y + camera.y
end

function Tilemap:update(_) -- delta_time
end

function Tilemap:draw()
  self:prepare_()

  local camera = self.camera
  Canvas.push()
  Canvas.clipping(camera.screen_x, camera.screen_y, camera.width, camera.height)

  local ox, oy = camera.screen_x + camera.column_offset, camera.screen_y + camera.row_offset
  for _, v in ipairs(self.batch) do
    local cell_id, cell_x, cell_y = table.unpack(v)
    self.bank:blit(math.tointeger(cell_id), cell_x + ox, cell_y + oy)
  end

  Canvas.pop()
end

function Tilemap:prepare_()
  if self.batch then -- Check if we need to rebuild the bank batch.
    return
  end

  local camera = self.camera

  local cw, ch = self.bank:cell_width(), self.bank:cell_height()

  local rows = math.min(self.grid:width() - camera.start_row, camera.rows + 1) -- We handle an additional row/column
  local columns = math.min(self.grid:height() - camera.start_column, camera.columns + 1) -- for sub-tile scrolling

  local batch = {}
  local y = 0
  local r = camera.start_row
  for _ = 1, rows do
    local x = 0
    local c = camera.start_column
    for _ = 1, columns do
      local cell_id = self.grid:peek(c, r)
      table.insert(batch, { cell_id, x, y })
      x = x + cw
      c = c + 1
    end
    y = y + ch
    r = r + 1
  end
  self.batch = batch
end

function Tilemap:to_string()
  local camera = self.camera

  return string.format("%.0f %0.f | %.0f %0.f | %0.f %0.f",
    camera.x, camera.y,
    camera.start_column, camera.start_row,
    camera.column_offset, camera.row_offset)
end

return Tilemap