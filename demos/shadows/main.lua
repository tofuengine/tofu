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
local Canvas = require("tofu.graphics").Canvas
local Display = require("tofu.graphics").Display
local Palette = require("tofu.graphics").Palette
local Font = require("tofu.graphics").Font

local Main = Class.define()

local SPEED = 32

--384 x 224 pixels

function Main:__ctor()
  local greyscale = Palette.new(256)
  local palette = Palette.new(3, 3, 2) -- R3G3B2
  Display.palette(palette)

  local canvas = Canvas.default()
  canvas:transparent(0, false)

  self.background = Canvas.new("assets/background.png")
  self.stamp = Canvas.new("assets/sphere.png", 0, greyscale)
  self.bank = Bank.new(Canvas.new("assets/sheet.png",
    palette:match(0, 0, 0), palette:match(255, 255, 255)), 32, 32)
  self.font = Font.default(palette:match(255, 255, 255), palette:match(0, 0, 0))
  self.velocity = { x = 0, y = 0 }
  self.position = { x = 0, y = 0 }
  self.cursor = { x = 0, y = 0 }
end

function Main:input()
  if Input.is_down("up") then
    self.velocity.y = -SPEED
  elseif Input.is_down("down") then
    self.velocity.y = SPEED
  else
    self.velocity.y = 0
  end
  if Input.is_down("left") then
    self.velocity.x = -SPEED
  elseif Input.is_down("right") then
    self.velocity.x = SPEED
  else
    self.velocity.x = 0
  end
  if Input.is_down("a") then
    self.apply = true
  else
    self.apply = false
  end
end

function Main:update(delta_time)
  self.position.x = self.position.x + self.velocity.x * delta_time
  self.position.y = self.position.y + self.velocity.y * delta_time

  self.cursor.x, self.cursor.y = Input.cursor()
end

function Main:render(_)
  local canvas = Canvas.default()
  canvas:clear()

--  local time = System.time()

  if self.apply then
    local w, h = self.stamp:size()
    canvas:blend(self.cursor.x - (w * 0.5), self.cursor.y - (h * 0.5), self.stamp, "add", self.background)
  end

  canvas:copy(self.background)

  canvas:push()
    canvas:transparent(255, true)
    self.bank:blit(canvas, self.position.x, self.position.y, 0)
    self.font:write(canvas, 0, 0,string.format("FPS: %.1f", System.fps()))
  canvas:pop()

  canvas:square("fill", self.cursor.x - 4, self.cursor.y - 4, 8, 0)
end

return Main
