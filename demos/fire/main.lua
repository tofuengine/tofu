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

local Class = require("tofu.core.class")
local System = require("tofu.core.system")
local Input = require("tofu.events.input")
local Canvas = require("tofu.graphics.canvas")
local Display = require("tofu.graphics.display")
local Font = require("tofu.graphics.font")
local Palette = require("tofu.graphics.palette")
local Shape = require("tofu.graphics.shape")
local Grid = require("tofu.util.grid")

local STEPS = 64
local PALETTE = {
    { 0x00, 0x00, 0x00 }, { 0x24, 0x00, 0x00 }, { 0x48, 0x00, 0x00 }, { 0x6D, 0x00, 0x00 },
    { 0x91, 0x00, 0x00 }, { 0xB6, 0x00, 0x00 }, { 0xDA, 0x00, 0x00 }, { 0xFF, 0x00, 0x00 },
    { 0xFF, 0x3F, 0x00 }, { 0xFF, 0x7F, 0x00 }, { 0xFF, 0xBF, 0x00 }, { 0xFF, 0xFF, 0x00 },
    { 0xFF, 0xFF, 0x3F }, { 0xFF, 0xFF, 0x7F }, { 0xFF, 0xFF, 0xBF }, { 0xFF, 0xFF, 0xFF }
  }

local Main = Class.define()

function Main:__ctor()
  Display.palette(Palette.new(PALETTE))

  local canvas = Canvas.default()
  local width, height = canvas:image():size()

  self.font = Font.default(0, 15)
  self.x_size = width / STEPS
  self.y_size = height / STEPS
  self.windy = false
  self.damping = 1.0
  self.grid = Grid.new(STEPS, STEPS, { 0 })

  self:reset()
end

function Main:reset()
  self.grid:process(function(column, row, _)
    if row == STEPS - 1 then
      return column, row, #PALETTE - 1
    end
    return column, row, 0
  end)
end

function Main:process()
  if Input.is_pressed("select") then
    self.windy = not self.windy
  elseif Input.is_pressed("left") then
    self.damping = self.damping - 0.1
  elseif Input.is_pressed("right") then
    self.damping = self.damping + 0.1
  elseif Input.is_pressed("start") then
    self:reset()
  end
end

function Main:update(_)
  local windy = self.windy
  self.grid:process(function(column, row, value)
      if row == 0 then -- Skip the 1st row entirely.
        return column, row, value
      end

      if value > 0 then
        local delta = math.random(0, 1) * self.damping
        value = value - delta
        if value < 0 then
          value = 0
        end
      end

      if windy then
        column = column + math.random(0, 3) - 1
        if column < 0 then
          column = 0
        elseif column > STEPS - 1 then
          column = STEPS - 1
        end
      end

      return column, row - 1, value -- Move up one row!
    end)
end

function Main:render(_)
  local canvas = Canvas.default()

  local image = canvas:image()
  local width, _ = image:size()
  image:clear(0)


  local w = self.x_size
  local h = self.y_size
  self.grid:scan(function(column, row, value)
      local x = column * w
      local y = row * h
      if value > 0 then
        Shape.rectangle(canvas, "fill", x, y, w, h, value)
      end
    end)

    self.font:write(canvas, 0, 0, string.format("FPS: %d", System.fps()))
    self.font:write(canvas, width, 0, string.format("D: %.2f", self.damping), "right")
end

return Main
