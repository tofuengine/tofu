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
local Input = require("tofu.events").Input
local Bank = require("tofu.graphics").Bank
local Batch = require("tofu.graphics").Batch
local Canvas = require("tofu.graphics").Canvas
local Display = require("tofu.graphics").Display
local Font = require("tofu.graphics").Font
local Palette = require("tofu.graphics").Palette

local Main = Class.define()

function Main:__ctor()
  Display.palette(Palette.new("pico-8-ext"))

  local canvas = Canvas.default()
  canvas:transparent({ ["0"] = false, ["22"] = true })
  canvas:background(12)

  self.bank = Bank.new(Canvas.new("assets/sprites.png", 22), 16, 16)
  self.batch = Batch.new(self.bank, 5000)
  self.font = Font.default(22, 2)

  self.x, self.y = 0, 0
  self.x_speed, self.y_speed = 0, 0
  self.scale = 7.0
  self.scale_speed = 0.0
  self.flip_x = true
  self.flip_y = false
end

function Main:process()
  self.y_speed = 0
  if Input.is_down("up") then
    self.y_speed = self.y_speed - 16
  end
  if Input.is_down("down") then
    self.y_speed = self.y_speed + 16
  end

  self.x_speed = 0
  if Input.is_down("left") then
    self.x_speed = self.x_speed - 16
  end
  if Input.is_down("right") then
    self.x_speed = self.x_speed + 16
  end

  self.scale_speed = 0
  if Input.is_down("a") then
    self.scale_speed = self.scale_speed + 2
  end
  if Input.is_down("b") then
    self.scale_speed = self.scale_speed - 2
  end

  if Input.is_pressed("x") then
    self.flip_x = not self.flip_x
  end
  if Input.is_pressed("y") then
    self.flip_y = not self.flip_y
  end
end

function Main:update(delta_time)
  self.scale = math.max(0, self.scale + self.scale_speed * delta_time)
  self.x = self.x + self.x_speed * delta_time
  self.y = self.y + self.y_speed * delta_time
end

function Main:render(_)
  local canvas = Canvas.default()
  canvas:clear()

  local width, height = canvas:size()
  local x, y = self.x, self.y

  for _ = 1, 1 do
    self.bank:blit(canvas, x, y, 9,
      self.flip_x and -self.scale or self.scale, self.flip_y and -self.scale or self.scale)
  end

  self.font:write(canvas, 0, 0, string.format("FPS: %d", math.floor(System.fps() + 0.5)))
  self.font:write(canvas, width, height, string.format("S:%.2f X:%s Y:%s",
    self.scale, self.flip_x, self.flip_y), "right", "bottom")
end

return Main