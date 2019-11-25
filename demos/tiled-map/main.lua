local Grid = require("tofu.collections").Grid
local System = require("tofu.core").System
local Bank = require("tofu.graphics").Bank
local Canvas = require("tofu.graphics").Canvas
local Font = require("tofu.graphics").Font
local Input = require("tofu.events").Input
local File = require("tofu.io").File
local Class = require("tofu.util").Class

local Main = Class.define()

local CAMERA_SPEED = 64.0

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
--[[
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
--]]--
local function split(s, sep)
  local fields = {}
  local pattern = string.format("([^%s]+)", sep or " ")
  string.gsub(s, pattern, function(c) fields[#fields + 1] = c end)
  return fields
end

function Tilemap:__ctor(file, camera_columns, camera_rows, camera_alignment) -- TODO: pass a camera easing function.
  local content = File.read(file)
  local cells = {}
  local tokens = {}
  for chunk in string.gmatch(content, "[^\n]+") do
    table.insert(tokens, chunk)
  end
  for i = 6, #tokens do
    local chunk = tokens[i]
    for cell in string.gmatch(chunk, "[^ ]+") do
      table.insert(cells, tonumber(cell))
    end
  end

  self.bank = Bank.new(tokens[1], tonumber(tokens[2]), tonumber(tokens[3]))
  self.grid = Grid.new(tonumber(tokens[4]), tonumber(tokens[5]), cells)
  self.batch = {}
  self.font = Font.default(0, 3)

  self:camera(camera_columns, camera_rows, camera_alignment)
end

function Tilemap:camera(camera_columns, camera_rows, camera_alignment)
  local alignments = split(camera_alignment, "-")

  self.camera_columns = camera_columns
  self.camera_rows = camera_rows
  self.camera_width = camera_rows * self.bank:cell_width()
  self.camera_height = camera_columns * self.bank:cell_height()
  self.camera_alignment_x = CAMERA_ALIGNMENT_MULTIPLIERS[alignments[1]] * -self.camera_width
  self.camera_alignment_y = CAMERA_ALIGNMENT_MULTIPLIERS[alignments[2]] * -self.camera_height
  self.camera_min_x = -self.camera_alignment_x
  self.camera_max_x = (self.grid:width() - camera_columns) * self.bank:cell_width() + self.camera_alignment_x
  self.camera_min_y = -self.camera_alignment_y
  self.camera_max_y = (self.grid:height() - camera_rows) * self.bank:cell_height() + self.camera_alignment_y

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

  self.camera_start_column = math.floor(map_x / self.bank:cell_width())
  self.camera_start_row = math.floor(map_y / self.bank:cell_height())
  self.camera_offset_x = -(map_x % self.bank:cell_width())
  self.camera_offset_y = -(map_y % self.bank:cell_height())

  self.modified = (camera_start_column ~= self.camera_start_column) or (camera_start_row ~= self.camera_start_row)
end

function Tilemap:update(_) -- delta_time
  -- TODO: update the camera position in the case we are performing easings and/or following the user
  -- (or some more advanced techniques).

  if self.modified then -- Check if we need to rebuild the bank batch.
    self.modified = false

    for i = 0, self.camera_rows do -- inclusive, we handle an additional row to enable scrolling offset.
      local y = i * self.bank:cell_height()
      local r = self.camera_start_row + i
      for j = 0, self.camera_columns do
        local x = j * self.bank:cell_width()
        local c = self.camera_start_column + j
        local cell_id = self.grid:peek(c, r)
        table.insert(self.batch, { cell_id, x, y })
      end
    end
  end
end

function Tilemap:render(_) -- ratio
  for _, v in pairs(self.batch) do
    local cell_id, x, y = table.unpack(v)
    self.bank:blit(math.tointeger(cell_id), x + self.camera_offset_x, y + self.camera_offset_y)
  end

  self.font:write(string.format("%.0f %0.f", self.camera_x, self.camera_y), Canvas.width(), 0, "right")
  self.font:write(string.format("FPS: %d", System.fps()), 0, 0, "left")
end

function Main:__ctor()
  Canvas.palette("gameboy")

  self.map = Tilemap.new("assets/world.map", 15, 10, "left-top")
  self.map:move_to(160, 160)
end

function Main:input()
  self.dx = 0
  self.dy = 0
  if Input.is_key_down(Input.LEFT) then
    self.dx = self.dx - 1
  end
  if Input.is_key_down(Input.RIGHT) then
    self.dx = self.dx + 1
  end
  if Input.is_key_down(Input.UP) then
    self.dy = self.dy - 1
  end
  if Input.is_key_down(Input.DOWN) then
    self.dy = self.dy + 1
  end
end

function Main:update(delta_time)
  local dx = self.dx * CAMERA_SPEED * delta_time
  local dy = self.dy * CAMERA_SPEED * delta_time
  if dx ~= 0.0 or dy ~= 0.0 then
    self.map:scroll_by(dx, dy)
  end
  self.map:update(delta_time)
end

function Main:render(ratio)
  Canvas.clear()
  self.map:render(ratio)
end

return Main