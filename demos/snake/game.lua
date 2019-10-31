local Grid = require("tofu.collections").Grid
local Canvas = require("tofu.graphics").Canvas
local Font = require("tofu.graphics").Font
local Input = require("tofu.events").Input
local Class = require("tofu.util").Class
local System = require("tofu.core").System

local INITIAL_LENGHT = 5
local SPEED_RATIO = 5e
local CELLS = 32
local LIFE = 1.0
local SPEED = 5.0

local DELTA_X = { up = 0, down = 0, left = -1, right = 1 }
local DELTA_Y = { up = -1, down = 1, left = 0, right = 0 }

local MAP =  "********************************"
          .. "*                              *"
          .. "*                              *"
          .. "*                              *"
          .. "*                              *"
          .. "*                              *"
          .. "*                              *"
          .. "*                              *"
          .. "*                              *"
          .. "*                              *"
          .. "*                              *"
          .. "*                              *"
          .. "*                              *"
          .. "*                              *"
          .. "*                              *"
          .. "*                              *"
          .. "*                              *"
          .. "*                              *"
          .. "*                              *"
          .. "*                              *"
          .. "*                              *"
          .. "*                              *"
          .. "*                              *"
          .. "*                              *"
          .. "*                              *"
          .. "*                              *"
          .. "*                              *"
          .. "*                              *"
          .. "*                              *"
          .. "*                              *"
          .. "*                              *"
          .. "********************************"


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
dump(self)

  self:reset()
end

function Game:draw_map(map)
  for i = 1, map:len() do
    local c = map:byte(i)
    local row = (i - 1) % CELLS
    local column = (i - 1) / CELLS
    local value = 0
    if c == 42 then
      value = -1
    end
    self.grid:poke(column, row, value)
  end
end

function Game:reset()
  self.length = INITIAL_LENGHT
  self.state = "running"

  self:draw_map(MAP)

  self.position = { x = CELLS / 2, y = CELLS / 2 }
  self.accumulator = 0.0
  for i = 1, self.length do
    self.grid:poke(self.position.x - self.length + i, self.position.y, i * LIFE)
  end
  self.direction = "right"

  self:generate_food()
end

function Game:input()
  if self.state == "game-over" then
    if Input.is_key_pressed(Input.START) then
      self:reset()
    end
    return
  end

  if not self.can_move then
    return
  end
  if Input.is_key_pressed(Input.UP) then
    if self.direction ~= "down" then
      self.direction = "up"
      self.can_move = false
    end
  elseif Input.is_key_pressed(Input.DOWN) then
    if self.direction ~= "up" then
      self.direction = "down"
      self.can_move = false
    end
  elseif Input.is_key_pressed(Input.LEFT) then
    if self.direction ~= "right" then
      self.direction = "left"
      self.can_move = false
    end
  elseif Input.is_key_pressed(Input.RIGHT) then
    if self.direction ~= "left" then
      self.direction = "right"
      self.can_move = false
    end
  end
end

function Game:generate_food()
  while true do
    local column = math.random(0, CELLS - 1)
    local row = math.random(0, CELLS - 1)
    local value = self.grid:peek(column, row)
    if value == 0 then
      self.grid:poke(column, row, -2)
      break
    end
  end
end

function Game:update(delta_time)
  if self.state == "game-over" then
    return
  end

  local speed = SPEED * (self.length / SPEED_RATIO)

  self.grid:process(function(column, row, value)
      if value <= 0 then
        return column, row, value
      end
      value = math.max(value - speed * delta_time, 0)
      return column, row, value
    end)

  self.accumulator = self.accumulator + speed * delta_time
  while self.accumulator >= LIFE do
    self.can_move = true

    self.accumulator = self.accumulator - LIFE

    self.position.x = (self.position.x + DELTA_X[self.direction]) % CELLS
    self.position.y = (self.position.y + DELTA_Y[self.direction]) % CELLS

    local value = self.grid:peek(self.position.x, self.position.y)

    self.grid:poke(self.position.x, self.position.y, self.length * LIFE)

    if value == -2 then
      self.length = self.length + 1
      self:generate_food()
    elseif value ~= 0 then
      self.state = "game-over"
    end
  end
end

function Game:render(_)
  Canvas.clear()

  local w = self.x_size
  local h = self.y_size
  self.grid:scan(function(column, row, value)
      local x = column * w
      local y = row * h
      if value == -2 then
        Canvas.rectangle("fill", x, y, w, h, 3)
      elseif value == -1 then
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
