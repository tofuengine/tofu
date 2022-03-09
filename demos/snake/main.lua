--[[
MIT License

Copyright (c) 2019-2022 Marco Lizza

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

local Class = require("tofu.core.class")
local System = require("tofu.core.system")
local Canvas = require("tofu.graphics.canvas")
local Display = require("tofu.graphics.display")
local Font = require("tofu.graphics.font")
local Palette = require("tofu.graphics.palette")
local Controller = require("tofu.input.controller")
local Source = require("tofu.sound.source")
local Grid = require("tofu.util.grid")

local INITIAL_LENGTH <const> = 5
local SPEED_RATIO <const> = 5
local CELL_SIZE <const> = 8
local LIFE <const> = 1.0
local SPEED <const> = 5.0

local DELTA_X <const> = { up = 0, down = 0, left = -1, right = 1 }
local DELTA_Y <const> = { up = -1, down = 1, left = 0, right = 0 }

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

local MAP <const> = "48|28|49:-1|46:0|2:-1|46:0|2:-1|46:0|2:-1|46:0|2:-1|46:0|2:-1|46:0|2:-1|46:0|2:-1|46:0\z
  |2:-1|46:0|2:-1|46:0|2:-1|46:0|2:-1|46:0|2:-1|46:0|2:-1|46:0|2:-1|46:0|2:-1|46:0|2:-1|46:0|2:-1|46:0\z
  |2:-1|46:0|2:-1|46:0|2:-1|46:0|2:-1|46:0|2:-1|46:0|2:-1|46:0|2:-1|46:0|2:-1|46:0|49:-1"

local SOURCES <const> = {
    { id = "hit", name = "assets/sounds/hit.flac", type = "sample" },
    { id = "eat", name = "assets/sounds/eat.flac", type = "sample" },
    { id = "blip", name = "assets/sounds/blip.flac", type = "sample" },
  }

local Main = Class.define()

function Main:__ctor()
  local palette = Palette.default("gameboy")
  Display.palette(palette)

  local canvas = Canvas.default()
  local image = canvas:image()
  local width, height = image:size()

  self.font = Font.default(palette:match(0, 0, 0), palette:match(255, 255, 255))
  self.grid = Grid.new(width // CELL_SIZE, height // CELL_SIZE, { 0 })

  self.sources = {}
  for _, source in ipairs(SOURCES) do
    local instance = Source.new(source.name, source.type)
    self.sources[source.id] = { instance = instance, pan = 0.0, balance = 0.0 }
  end

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
  local controller = Controller.default()

  if self.state == "game-over" then
    if controller:is_pressed("start") then
      self:reset()
    end
    return
  end

  if not self.can_move then
    return
  end
  if controller:is_pressed("up") then
    if self.direction ~= "down" then
      self.direction = "up"
      self.can_move = false
    end
  elseif controller:is_pressed("down") then
    if self.direction ~= "up" then
      self.direction = "down"
      self.can_move = false
    end
  elseif controller:is_pressed("left") then
    if self.direction ~= "right" then
      self.direction = "left"
      self.can_move = false
    end
  elseif controller:is_pressed("right") then
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
    local x = (self.position.x + DELTA_X[self.direction]) % gw
    local y = (self.position.y + DELTA_Y[self.direction]) % gh

    local value = self.grid:peek(x, y)

    if value == -2 then
      self.sources["eat"].instance:play()
      self.length = self.length + 1
      self:generate_food()
    elseif value ~= 0 then
      self.sources["hit"].instance:play()
      self.state = "game-over"
      break
    end

    self.grid:poke(x, y, self.length * LIFE)
    self.position.x = x
    self.position.y = y
  end
end

function Main:render(_)
  local canvas = Canvas.default()
  local image = canvas:image()
  local width, height = image:size()
  image:clear(0)

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

    if self.state == "game-over" then
      local points <const> = (self.length - INITIAL_LENGTH) * 10
      canvas:write(width * 0.5, height * 0.25, self.font,
        "GAME OVER", "center", "middle", 4, 4)
      canvas:write(width * 0.5, height * 0.50, self.font,
        string.format("Your final score is %d", points), "center", "middle", 2, 2)
      canvas:write(width * 0.5, height * 0.75, self.font,
        "-- press start --", "center", "middle", 2, 2)
    end

    canvas:write(0, 0, self.font, string.format("FPS: %d", System.fps()))
end

return Main
