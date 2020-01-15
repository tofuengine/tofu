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

local Grid = require("tofu.collections").Grid
local Canvas = require("tofu.graphics").Canvas
local Font = require("tofu.graphics").Font
local Input = require("tofu.events").Input
local Class = require("tofu.util").Class
local System = require("tofu.core").System

local INITIAL_LENGHT = 5
local SPEED_RATIO = 5
local CELL_SIZE = 8
local LIFE = 1.0
local SPEED = 5.0

local DELTA_X = { up = 0, down = 0, left = -1, right = 1 }
local DELTA_Y = { up = -1, down = 1, left = 0, right = 0 }

local MAP =  "************************************************"
          .. "*                                              *"
          .. "*                                              *"
          .. "*                                              *"
          .. "*                                              *"
          .. "*                                              *"
          .. "*                                              *"
          .. "*                                              *"
          .. "*                                              *"
          .. "*                                              *"
          .. "*                                              *"
          .. "*                                              *"
          .. "*                                              *"
          .. "*                                              *"
          .. "*                                              *"
          .. "*                                              *"
          .. "*                                              *"
          .. "*                                              *"
          .. "*                                              *"
          .. "*                                              *"
          .. "*                                              *"
          .. "*                                              *"
          .. "*                                              *"
          .. "*                                              *"
          .. "*                                              *"
          .. "*                                              *"
          .. "*                                              *"
          .. "************************************************"

local Main = Class.define()

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

function Main:__ctor()
  Canvas.palette("gameboy")

  self.font = Font.default(0, 3)
  self.grid = Grid.new(math.tointeger(Canvas.width() / CELL_SIZE), math.tointeger(Canvas.height() / CELL_SIZE), 0)
dump(self)

  self:reset()
end

function Main:draw_map(map)
  for i = 1, map:len() do
    local c = map:byte(i)
    local column = (i - 1) % self.grid:width()
    local row = (i - 1) / self.grid:width()
    local value = 0
    if c == 42 then
      value = -1
    end
    self.grid:poke(column, row, value)
  end
end

function Main:reset()
  self.length = INITIAL_LENGHT
  self.state = "running"

  self:draw_map(MAP)

  self.position = { x = self.grid:width() / 2, y = self.grid:height() / 2 }
  self.accumulator = 0.0
  for i = 1, self.length do
    self.grid:poke(self.position.x - self.length + i, self.position.y, i * LIFE)
  end
  self.direction = "right"

  self:generate_food()
end

function Main:input()
  if self.state == "game-over" then
    if Input.is_pressed("START") then
      self:reset()
    end
    return
  end

  if not self.can_move then
    return
  end
  if Input.is_pressed("UP") then
    if self.direction ~= "down" then
      self.direction = "up"
      self.can_move = false
    end
  elseif Input.is_pressed("DOWN") then
    if self.direction ~= "up" then
      self.direction = "down"
      self.can_move = false
    end
  elseif Input.is_pressed("LEFT") then
    if self.direction ~= "right" then
      self.direction = "left"
      self.can_move = false
    end
  elseif Input.is_pressed("RIGHT") then
    if self.direction ~= "left" then
      self.direction = "right"
      self.can_move = false
    end
  end
end

function Main:generate_food()
  while true do
    local column = math.random(0, self.grid:width() - 1)
    local row = math.random(0, self.grid:height() - 1)
    local value = self.grid:peek(column, row)
    if value == 0 then
      self.grid:poke(column, row, -2)
      break
    end
  end
end

function Main:update(delta_time)
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

    self.position.x = (self.position.x + DELTA_X[self.direction]) % self.grid:width()
    self.position.y = (self.position.y + DELTA_Y[self.direction]) % self.grid:height()

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

function Main:render(_)
  Canvas.clear()

  self.grid:scan(function(column, row, value)
      local x = column * CELL_SIZE
      local y = row * CELL_SIZE
      if value == -2 then
        Canvas.rectangle("fill", x, y, CELL_SIZE, CELL_SIZE, 3)
      elseif value == -1 then
        Canvas.rectangle("fill", x, y, CELL_SIZE, CELL_SIZE, 1)
      elseif value ~= 0 then
        Canvas.rectangle("fill", x, y, CELL_SIZE, CELL_SIZE, 2)
      end
    end)

    self.font:write(string.format("FPS: %d", System.fps()), 0, 0)
end

return Main
