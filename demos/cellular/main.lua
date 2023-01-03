--[[
MIT License

Copyright (c) 2019-2023 Marco Lizza

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
local Controller = require("tofu.input.controller")
local Cursor = require("tofu.input.cursor")
local Canvas = require("tofu.graphics.canvas")
local Display = require("tofu.graphics.display")
local Font = require("tofu.graphics.font")
local Palette = require("tofu.graphics.palette")
local Grid = require("tofu.util.grid")

local Main = Class.define()

local PALETTE <const> = Palette.default("famicube")
local SAND <const> = PALETTE:match(255, 255, 0)
local CURSOR <const> = PALETTE:match(255, 255, 255)

function Main:__ctor()
  Display.palette(PALETTE)

  local canvas = Canvas.default()
  local image = canvas:image()
  local width, height = image:size()

  self.font = Font.default(0, 15)
  self.grid = Grid.new(width, height, { 0 })
  self.dirg = Grid.new(width, height, { 0 })
end

function Main:process()
  local controller <const> = Controller.default()
  if controller:is_pressed("select") then
    self.windy = not self.windy
  elseif controller:is_pressed("left") then
    self.damping = self.damping - 0.1
  elseif controller:is_pressed("right") then
    self.damping = self.damping + 0.1
  elseif controller:is_pressed("start") then
    self:reset()
  end

  local cursor <const> = Cursor.default()
  if cursor:is_down("left") then
    local x, y = cursor:position()
    self.grid:poke(x, y, 1)
  end
end

function Main:update(_)
  self.dirg:copy(self.grid)

  local width, height = self.dirg:size()

  for y = 0, height - 2 do
    for x = 1, width - 2 do
      if self.dirg:peek(x, y) == 1 then
        if self.dirg:peek(x, y + 1) == 0 then
          self.grid:poke(x, y, 0)
          self.grid:poke(x, y + 1, 1)
        elseif self.dirg:peek(x - 1, y + 1) == 0 then
          self.grid:poke(x, y, 0)
          self.grid:poke(x - 1, y + 1, 1)
        elseif self.dirg:peek(x + 1, y + 1) == 0 then
          self.grid:poke(x, y, 0)
          self.grid:poke(x + 1, y + 1, 1)
        end
      end
    end
  end
end

local function draw_cursor(canvas, x, y, size, color)
  canvas:line(x, y - size, x, y + size, color)
  canvas:line(x - size, y, x + size, y, color)
end

function Main:render(_)
  local canvas = Canvas.default()
  local image = canvas:image()
  image:clear(0)

  self.grid:scan(function(column, row, value)
      if value == 1 then
        canvas:point(column, row, SAND)
      end
    end)

  local cursor <const> = Cursor.default()
  local cx, cy = cursor:position()
  draw_cursor(canvas, cx, cy, 2, CURSOR)

  canvas:write(0, 0, self.font, string.format("FPS: %d", System.fps()))
end

return Main
