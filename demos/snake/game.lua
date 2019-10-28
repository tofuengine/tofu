local Grid = require("tofu.collections").Grid
local Canvas = require("tofu.graphics").Canvas
local Font = require("tofu.graphics").Font
local Input = require("tofu.events").Input
local Class = require("tofu.util").Class
local System = require("tofu.core").System

local CELLS = 32
local LIFE = 1.0
local SPEED = 5.0

local DELTA_X = { up = 0, down = 0, left = -1, right = 1 }
local DELTA_Y = { up = -1, down = 1, left = 0, right = 0 }

local Game = Class.define()

local function dump(t, spaces) -- TODO: add dump function to core? util?
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

function Game:__ctor()
  Canvas.palette("gameboy")

  self.font = Font.default(0, 3)
  self.x_size = Canvas.width() / CELLS
  self.y_size = Canvas.height() / CELLS
  self.grid = Grid.new(CELLS, CELLS, 0)
  self.length = 5
dump(self)

  self:reset()
end

function Game:reset()
  self.position = { x = math.floor(CELLS / 2), y = math.floor(CELLS / 2) }
  self.accumulator = 0.0
  self.grid:fill(0)
  for i = 1, self.length do
    self.grid:poke(self.position.x - self.length + i, self.position.y, i * LIFE)
  end
  for row = 0, CELLS - 1 do
    for column = 0, CELLS - 1 do
      if row == 0 or column == 0 or row == CELLS -1 or column == CELLS - 1 then
        self.grid:poke(column, row, -1)
      end
    end
  end
  self.direction = "right"
end

function Game:input()
  if Input.is_key_pressed(Input.UP) then
    if self.direction ~= "down" then
      self.direction = "up"
    end
  elseif Input.is_key_pressed(Input.DOWN) then
    if self.direction ~= "up" then
      self.direction = "down"
    end
  elseif Input.is_key_pressed(Input.LEFT) then
    if self.direction ~= "right" then
      self.direction = "left"
    end
  elseif Input.is_key_pressed(Input.RIGHT) then
    if self.direction ~= "left" then
      self.direction = "right"
    end
  elseif Input.is_key_pressed(Input.SELECT) then
    self:reset()
  end
end

function Game:update(delta_time)
  self.grid:process(function(column, row, value)
      if value <= 0 then
        return column, row, value
      end
      value = math.max(value - SPEED * delta_time, 0)
      return column, row, value
    end)

  self.accumulator = self.accumulator + SPEED * delta_time
  while self.accumulator >= LIFE do
    self.accumulator = self.accumulator - LIFE
    self.position.x = self.position.x + DELTA_X[self.direction]
    self.position.y = self.position.y + DELTA_Y[self.direction]
    -- collides(position)
    self.grid:poke(self.position.x, self.position.y, self.length * LIFE)
  end
end

function Game:render(_)
  Canvas.clear()

  local w = self.x_size
  local h = self.y_size
  self.grid:scan(function(column, row, value)
      local x = column * w
      local y = row * h
      if value == -1 then
        Canvas.rectangle("fill", x, y, w, h, 1)
      elseif value == 0 then
        Canvas.rectangle("fill", x, y, w, h, 0)
      else
        Canvas.rectangle("fill", x, y, w, h, 2)
      end
    end)

    self.font:write(string.format("FPS: %d", System.fps()), 0, 0, "left")
end

return Game
