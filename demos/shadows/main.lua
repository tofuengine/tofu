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
  local palette = Palette.new("famicube")
  Display.palette(palette)

  local canvas = Canvas.default()
  canvas:transparent(palette:color_to_index(255, 255, 255), true)
  canvas:transparent(palette:color_to_index(0, 0, 0), false)

  self.background = Canvas.new("assets/background.png")
  self.bank = Bank.new(canvas, Canvas.new("assets/sheet.png",
  palette:color_to_index(0, 0, 0), palette:color_to_index(255, 255, 255)), 32, 32)
  self.font = Font.default(canvas, palette:color_to_index(255, 255, 255), palette:color_to_index(0, 0, 0))
  self.velocity = { x = 0, y = 0 }
  self.position = { x = 0, y = 0 }
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
end

function Main:update(delta_time)
  self.position.x = self.position.x + self.velocity.x * delta_time
  self.position.y = self.position.y + self.velocity.y * delta_time
end

function Main:render(_)
  local canvas = Canvas.default()
--  canvas:clear()

--  local time = System.time()

  self.background:copy(canvas)
  self.bank:blit(0, self.position.x, self.position.y)

  self.font:write(string.format("FPS: %.1f", System.fps()), 0, 0)
end

return Main
