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

function Tilemap:__ctor(file, columns, rows)
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
  self.batch = nil

--  self.tile_width, self.tile_height = self.bank:cell_width(), self.bank:cell_height()
  self.aabb = {
      x0 = 0,
      y0 = 0,
      x1 = self.grid:width() * self.bank:cell_width() - 1,
      y1 = self.grid:height() * self.bank:cell_height() - 1
    }
  self.x, self.y = 0, 0 -- TODO: move them outside the map, they are the avatar coordinates!

  self:camera(columns, rows)
end

function Tilemap:camera(columns, rows)
  local camera = {}
  self.camera = camera

  camera.columns = columns
  camera.rows = rows
  camera.width = columns * self.bank:cell_width()
  camera.height = rows * self.bank:cell_height()

  self:center_at(self.camera.anchor_x or 0.5, self.camera.anchor_y or 0.5)
end

function Tilemap:center_at(anchor_x, anchor_y)
  local camera = self.camera
  camera.anchor_x = anchor_x
  camera.anchor_y = anchor_y
  camera.offset_x = anchor_x * camera.width
  camera.offset_y = anchor_y * camera.height
  camera.aabb = {
      x0 = camera.offset_x,
      y0 = camera.offset_y,
      x1 = self.grid:width() * self.bank:cell_width() - (camera.width - camera.offset_x) - 1,
      y1 = self.grid:height() * self.bank:cell_height() - (camera.height - camera.offset_y) - 1
    }

  self:move_to(self.x, self.y)
end

function Tilemap:scroll_by(dx, dy)
  self:move_to(self.x + dx, self.y + dy)
end

function Tilemap:to_screen(dx, dy, x, y)
  return x - self.camera.x + self.camera.offset_x + dx, y - self.camera.y + self.camera.offset_y + dy
end

function Tilemap:to_world(dx, dy, x, y)
  return x - dx - self.camera.offset_x + self.camera.x, y - dy - self.camera.offset_y + self.camera.y
end

function Tilemap:move_to(x, y)
  self.x, self.y = bound(x, y, self.aabb)

  local camera = self.camera
  camera.x, camera.y = bound(x, y, camera.aabb)

  local map_x = math.floor(camera.x - camera.offset_x)
  local map_y = math.floor(camera.y - camera.offset_y)

  local cw, ch = self.bank:cell_width(), self.bank:cell_height()
  local start_column = math.floor(map_x / cw)
  local start_row = math.floor(map_y / ch)
  local column_offset = -(map_x % cw)
  local row_offset = -(map_y % ch)

  if camera.start_column ~= start_column or camera.start_row ~= start_row then
    camera.start_column = start_column
    camera.start_row = start_row
    self.batch = nil
  end
  camera.column_offset = column_offset
  camera.row_offset = row_offset
end

function Tilemap:update(_) -- delta_time
end

function Tilemap:draw(x, y)
  self:prepare_()

  local camera = self.camera
  Canvas.push()
  Canvas.clipping(x, y, camera.width, camera.height)

  local ox, oy = x + camera.column_offset, y + camera.row_offset
  for _, v in ipairs(self.batch) do
    local cell_id, cell_x, cell_y = table.unpack(v)
    self.bank:blit(math.tointeger(cell_id), cell_x + ox, cell_y + oy)
    --Canvas.rectangle("line", cell_x, cell_y, 32, 32, 1)
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

  return string.format("%.0f %0.f | %.0f %0.f | %.0f %0.f | %0.f %0.f",
    self.x, self.y,
    camera.x, camera.y,
    camera.start_column, camera.start_row,
    camera.column_offset, camera.row_offset)
end

return Tilemap