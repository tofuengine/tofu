--[[
MIT License

Copyright (c) 2019-2021 Marco Lizza

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
local System = require("tofu.core").System
local Canvas = require("tofu.graphics").Canvas
local Display = require("tofu.graphics").Display
local Font = require("tofu.graphics").Font
local Palette = require("tofu.graphics").Palette
local Input = require("tofu.events").Input
local Grid = require("tofu.util").Grid

local INITIAL_LENGTH = 5
local SPEED_RATIO = 5
local CELL_SIZE = 8
local LIFE = 1.0
local SPEED = 5.0

local DELTA_X = { up = 0, down = 0, left = -1, right = 1 }
local DELTA_Y = { up = -1, down = 1, left = 0, right = 0 }

--[[
************************************************
*                                              *
*                                              *
*                                              *
*                                              *
*                                              *
*                                              *
*                                              *
*                                              *
*                                              *
*                                              *
*                                              *
*                                              *
*                                              *
*                                              *
*                                              *
*                                              *
*                                              *
*                                              *
*                                              *
*                                              *
*                                              *
*                                              *
*                                              *
*                                              *
*                                              *
*                                              *
************************************************
]]

local MAP = "48|28|49:-1|46:0|2:-1|46:0|2:-1|46:0|2:-1|46:0|2:-1|46:0|2:-1|46:0|2:-1|46:0|2:-1|46:0\z
  |2:-1|46:0|2:-1|46:0|2:-1|46:0|2:-1|46:0|2:-1|46:0|2:-1|46:0|2:-1|46:0|2:-1|46:0|2:-1|46:0|2:-1|46:0\z
  |2:-1|46:0|2:-1|46:0|2:-1|46:0|2:-1|46:0|2:-1|46:0|2:-1|46:0|2:-1|46:0|2:-1|46:0|49:-1"

local Main = Class.define()

function Main:__ctor()
  local palette = Palette.new("gameboy")
  Display.palette(palette)

  local canvas = Canvas.default()
  local width, height = canvas:size()

  self.font = Font.default(palette:match(0, 0, 0), palette:match(255, 255, 255))
  self.grid = Grid.new(width // CELL_SIZE, height // CELL_SIZE, { 0 })
--  Log.dump(self)

  self:reset()
end

function Main:reset()
  self.length = INITIAL_LENGTH
  self.state = "running"

  self.grid = Grid.parse(MAP)

  local gw, gh = self.grid:size()
  self.position = { x = gw / 2, y = gh / 2 }
  self.accumulator = 0.0
  for i = 1, self.length do
    self.grid:poke(self.position.x - self.length + i, self.position.y, i * LIFE)
  end
  self.direction = "right"

  self:generate_food()
end

function Main:process()
  if self.state == "game-over" then
    if Input.is_pressed("start") then
      self:reset()
    end
    return
  end

  if not self.can_move then
    return
  end
  if Input.is_pressed("up") then
    if self.direction ~= "down" then
      self.direction = "up"
      self.can_move = false
    end
  elseif Input.is_pressed("down") then
    if self.direction ~= "up" then
      self.direction = "down"
      self.can_move = false
    end
  elseif Input.is_pressed("left") then
    if self.direction ~= "right" then
      self.direction = "left"
      self.can_move = false
    end
  elseif Input.is_pressed("right") then
    if self.direction ~= "left" then
      self.direction = "right"
      self.can_move = false
    end
  end
end

function Main:generate_food()
  local gw, gh = self.grid:size()
  while true do
    local column = math.random(0, gw - 1)
    local row = math.random(0, gh - 1)
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

    local gw, gh = self.grid:size()
    self.position.x = (self.position.x + DELTA_X[self.direction]) % gw
    self.position.y = (self.position.y + DELTA_Y[self.direction]) % gh

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
  local canvas = Canvas.default()
  canvas:clear()

  self.grid:scan(function(column, row, value)
      local x = column * CELL_SIZE
      local y = row * CELL_SIZE
      if value == -2 then
        canvas:rectangle("fill", x, y, CELL_SIZE, CELL_SIZE, 3)
      elseif value == -1 then
        canvas:rectangle("fill", x, y, CELL_SIZE, CELL_SIZE, 1)
      elseif value ~= 0 then
        canvas:rectangle("fill", x, y, CELL_SIZE, CELL_SIZE, 2)
      end
    end)

    self.font:write(canvas, 0, 0, string.format("FPS: %d", System.fps()))
end

return Main
