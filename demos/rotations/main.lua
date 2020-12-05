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
local Bank = require("tofu.graphics").Bank
local Batch = require("tofu.graphics").Batch
local Canvas = require("tofu.graphics").Canvas
local Display = require("tofu.graphics").Display
local Font = require("tofu.graphics").Font

local Main = Class.define()

function Main:__ctor()
  Display.palette("pico-8-ext")

  local canvas = Canvas.default()
  canvas:transparent({ ["0"] = false, ["22"] = true })
  canvas:background(12)

  self.bank = Bank.new(canvas, Canvas.new("assets/sprites.png"), 16, 16)
  self.batch = Batch.new(self.bank, 5000)
  self.font = Font.default(canvas, 22, 2)

  self.anchor = 0.5
  self.anchor_speed = 0.0
  self.rotation = 128
  self.rotation_speed = 0.0
  self.scale = 3.0
  self.scale_speed = 0.0
  self.flip_x = false
  self.flip_y = false
end

function Main:input()
  self.scale_speed = 0
  if Input.is_down("up") then
    self.scale_speed = self.scale_speed + 2
  end
  if Input.is_down("down") then
    self.scale_speed = self.scale_speed - 2
  end

  self.rotation_speed = 0
  if Input.is_down("left") then
    self.rotation_speed = self.rotation_speed - 64
  end
  if Input.is_down("right") then
    self.rotation_speed = self.rotation_speed + 64
  end

  self.anchor_speed = 0
  if Input.is_down("a") then
    self.anchor_speed = self.anchor_speed - 1
  end
  if Input.is_down("b") then
    self.anchor_speed = self.anchor_speed + 1
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
  self.rotation = self.rotation + self.rotation_speed * delta_time
  self.anchor = math.min(1.0, math.max(0.0, self.anchor + self.anchor_speed * delta_time))
end

function Main:render(_)
  local canvas = Canvas.default()
  canvas:clear()

  local width, height = canvas:size()
  local x, y = canvas:center()

  for _ = 1, 100 do
    self.bank:blit(9, x, y,
      self.flip_x and -self.scale or self.scale, self.flip_y and -self.scale or self.scale,
      self.rotation,
      self.anchor, self.anchor)
  end

  self.font:write(string.format("FPS: %d", math.floor(System.fps() + 0.5)), 0, 0)
  self.font:write(self.font:align(string.format("S:%.2f|R:%4d|A:%.2f", self.scale, self.rotation, self.anchor),
    width, height, "right", "bottom"))
end

return Main