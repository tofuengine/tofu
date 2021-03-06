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

local Class = require("tofu.core").Class
local System = require("tofu.core").System
local Input = require("tofu.events").Input
local Canvas = require("tofu.graphics").Canvas
local Display = require("tofu.graphics").Display
local Font = require("tofu.graphics").Font
local Grid = require("tofu.util").Grid

local STEPS = 64
local PALETTE = {
    0xFF000000, 0xFF240000, 0xFF480000, 0xFF6D0000,
    0xFF910000, 0xFFB60000, 0xFFDA0000, 0xFFFF0000,
    0xFFFF3F00, 0xFFFF7F00, 0xFFFFBF00, 0xFFFFFF00,
    0xFFFFFF3F, 0xFFFFFF7F, 0xFFFFFFBF, 0xFFFFFFFF
  }

local Main = Class.define()

function Main:__ctor()
  Display.palette(PALETTE)

  local canvas = Canvas.default()
  local width, height = canvas:size()

  self.font = Font.default(canvas, 0, 15)
  self.x_size = width / STEPS
  self.y_size = height / STEPS
  self.windy = false
  self.damping = 1.0
  self.grid = Grid.new(STEPS, STEPS, 0)

  self:reset()
end

function Main:reset()
  self.grid:fill(0)
  self.grid:stride(0, STEPS - 1, #PALETTE - 1, STEPS)
end

function Main:input()
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
  canvas:clear()
  local width, _ = canvas:size()

  local w = self.x_size
  local h = self.y_size
  self.grid:scan(function(column, row, value)
      local x = column * w
      local y = row * h
      if value > 0 then
        canvas:rectangle("fill", x, y, w, h, value)
      end
    end)

    self.font:write(string.format("FPS: %d", System.fps()), 0, 0)
    self.font:write(self.font:align(string.format("D: %.2f", self.damping), width, 0, "right"))
end

return Main
