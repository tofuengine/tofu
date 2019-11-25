local Grid = require("tofu.collections").Grid
local Bank = require("tofu.graphics").Bank
local File = require("tofu.io").File
local Class = require("tofu.util").Class

local CAMERA_ALIGNMENT_MULTIPLIERS = {
  ["left"] = 0.0,
  ["top"] = 0.0,
  ["center"] = 0.5,
  ["middle"] = 0.5,
  ["right"] = 1.0,
  ["bottom"] = 1.0
}

-- https://developer.mozilla.org/en-US/docs/Games/Techniques/Tilemaps/Square_tilemaps_implementation:_Scrolling_maps

local Tilemap = Class.define()

function Tilemap:__ctor(file, camera_columns, camera_rows, camera_alignment) -- TODO: pass a camera easing function.
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

  self:camera(camera_columns, camera_rows, camera_alignment)
end

function Tilemap:camera(camera_columns, camera_rows, camera_alignment)
  self.camera_columns = camera_columns
  self.camera_rows = camera_rows
  self.camera_width = camera_columns * self.bank:cell_width()
  self.camera_height = camera_rows * self.bank:cell_height()
  self.camera_alignment_x = CAMERA_ALIGNMENT_MULTIPLIERS[camera_alignment["horizontal"]] * -self.camera_width
  self.camera_alignment_y = CAMERA_ALIGNMENT_MULTIPLIERS[camera_alignment["vertical"]] * -self.camera_height
  self.camera_min_x = -self.camera_alignment_x
  self.camera_max_x = (self.grid:width() - camera_columns - 1) * self.bank:cell_width() + self.camera_alignment_x
  self.camera_min_y = -self.camera_alignment_y
  self.camera_max_y = (self.grid:height() - camera_rows - 1) * self.bank:cell_height() + self.camera_alignment_y

  self.modified = true
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

    local batch = {}
    for i = 0, self.camera_rows do -- inclusive, we handle an additional row/column for sub-tile scrolling.
      local y = i * self.bank:cell_height()
      local r = self.camera_start_row + i
      for j = 0, self.camera_columns do
        local x = j * self.bank:cell_width()
        local c = self.camera_start_column + j
        local cell_id = self.grid:peek(c, r)
        table.insert(batch, { cell_id, x, y })
      end
    end
    self.batch = batch
  end
end

function Tilemap:render(_) -- ratio
  for _, v in ipairs(self.batch) do
    local cell_id, x, y = table.unpack(v)
    self.bank:blit(math.tointeger(cell_id), x + self.camera_offset_x, y + self.camera_offset_y)
  end
end

return Tilemap