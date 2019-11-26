local Grid = require("tofu.collections").Grid
local Bank = require("tofu.graphics").Bank
local Canvas = require("tofu.graphics").Canvas
local File = require("tofu.io").File
local Class = require("tofu.util").Class

-- https://developer.mozilla.org/en-US/docs/Games/Techniques/Tilemaps/Square_tilemaps_implementation:_Scrolling_maps

local function dump(t, spaces)
  spaces = spaces or ""
  for k, v in pairs(t) do
    print(spaces .. k .. " " .. type(v) .. " " .. tostring(v))
    if type(v) == "table" then
      if (k ~= "__index") then
        dump(v, spaces .. " ")
      end
    end
  end
end

local Tilemap = Class.define()

function Tilemap:__ctor(file, camera_columns, camera_rows) -- TODO: pass a camera easing function.
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
  self.batch = {}

--  self.tile_width, self.tile_height = self.bank:cell_width(), self.bank:cell_height()

  self:camera(camera_columns, camera_rows)
end

function Tilemap:camera(camera_columns, camera_rows)
  self.camera_columns = camera_columns
  self.camera_rows = camera_rows
  self.camera_width = camera_columns * self.bank:cell_width()
  self.camera_height = camera_rows * self.bank:cell_height()
dump(self)

  self:center_at(self.camera_anchor_x or 0.5, self.camera_anchor_y or 0.5)
end

function Tilemap:center_at(ax, ay)
  self.camera_anchor_x = ax
  self.camera_anchor_y = ay

  self.camera_alignment_x = ax * self.camera_width
  self.camera_alignment_y = ay * self.camera_height

  self.camera_min_x = self.camera_alignment_x
  self.camera_max_x = (self.grid:width() - self.camera_columns - 1) * self.bank:cell_width() - self.camera_alignment_x
  self.camera_min_y = self.camera_alignment_y
  self.camera_max_y = (self.grid:height() - self.camera_rows - 1) * self.bank:cell_height() - self.camera_alignment_y

  self:move_to(self.camera_x or self.camera_min_x, self.camera_y or self.camera_min_y)
end

function Tilemap:scroll_by(dx, dy)
  self:move_to(self.camera_x + dx, self.camera_y + dy)
end

function Tilemap:move_to(x, y)
  self.camera_x = math.min(math.max(x, self.camera_min_x), self.camera_max_x)
  self.camera_y = math.min(math.max(y, self.camera_min_y), self.camera_max_y)

  local map_x = math.floor(self.camera_x - self.camera_alignment_x)
  local map_y = math.floor(self.camera_y - self.camera_alignment_y)

  local camera_start_column = self.camera_start_column
  local camera_start_row = self.camera_start_row

  local cw, ch = self.bank:cell_width(), self.bank:cell_height()
  self.camera_start_column = math.floor(map_x / cw)
  self.camera_start_row = math.floor(map_y / ch)
  self.camera_offset_x = -(map_x % cw)
  self.camera_offset_y = -(map_y % ch)

  self.modified = (camera_start_column ~= self.camera_start_column) or (camera_start_row ~= self.camera_start_row)
end

function Tilemap:update(_) -- delta_time
  -- TODO: update the camera position in the case we are performing easings and/or following the user
  -- (or some more advanced techniques).

  if self.modified then -- Check if we need to rebuild the bank batch.
    self.modified = false

    local cw, ch = self.bank:cell_width(), self.bank:cell_height()

    local batch = {}
    local y = 0
    local r = self.camera_start_row
    for _ = 0, self.camera_rows do -- We handle an additional row/column for sub-tile scrolling.
      local x = 0
      local c = self.camera_start_column
      for _ = 0, self.camera_columns do
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
end

function Tilemap:draw(x, y)
  Canvas.push()
  Canvas.clipping(x, y, self.camera_width, self.camera_height)

  local ox, oy = x + self.camera_offset_x, y + self.camera_offset_y
  for _, v in ipairs(self.batch) do
    local cell_id, cell_x, cell_y = table.unpack(v)
    self.bank:blit(math.tointeger(cell_id), cell_x + ox, cell_y + oy)
    --Canvas.rectangle("line", cell_x, cell_y, 32, 32, 1)
  end

  Canvas.pop()
end

function Tilemap:to_string()
  return string.format("%.0f %0.f | %.0f %0.f | %0.f %0.f",
    self.camera_x, self.camera_y,
    self.camera_start_column, self.camera_start_row,
    self.camera_offset_x, self.camera_offset_y)
end

return Tilemap