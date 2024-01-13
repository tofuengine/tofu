--[[
MIT License

Copyright (c) 2019-2024 Marco Lizza

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
local Bank = require("tofu.graphics.bank")
local Canvas = require("tofu.graphics.canvas")
local Display = require("tofu.graphics.display")
local Image = require("tofu.graphics.image")
local Palette = require("tofu.graphics.palette")
local Font = require("tofu.graphics.font")

local Main = Class.define()

local SPEED <const> = 32

--384 x 224 pixels

function Main:__ctor()
  local greyscale = Palette.new(256)
  local palette = Palette.new(3, 3, 2) -- R3G3B2
  Display.palette(palette)

  local canvas = Canvas.default()
  canvas:transparent(0, false)

  self.background = Image.new("assets/background.png")
  self.stamp = Image.new("assets/sphere.png", 0, greyscale)
  self.bank = Bank.new(Image.new("assets/sheet.png",
    palette:match(0, 0, 0), palette:match(255, 255, 255)), 32, 32)
  self.font = Font.default(palette:match(255, 255, 255), palette:match(0, 0, 0))
  self.velocity = { x = 0, y = 0 }
  self.position = { x = 0, y = 0 }
  self.cursor = { x = 0, y = 0 }
end

function Main:process()
  local controller = Controller.default()
  if controller:is_down("up") then
    self.velocity.y = -SPEED
  elseif controller:is_down("down") then
    self.velocity.y = SPEED
  else
    self.velocity.y = 0
  end
  if controller:is_down("left") then
    self.velocity.x = -SPEED
  elseif controller:is_down("right") then
    self.velocity.x = SPEED
  else
    self.velocity.x = 0
  end
  if controller:is_down("a") then
    self.apply = true
  else
    self.apply = false
  end
end

function Main:update(delta_time)
  self.position.x = self.position.x + self.velocity.x * delta_time
  self.position.y = self.position.y + self.velocity.y * delta_time

  local cursor = Cursor.default()
  self.cursor.x, self.cursor.y = cursor:position()
end

function Main:render(_)
  local canvas = Canvas.default()
  local image = canvas:image()
  image:clear(0)

--  local time = System.time()

  if self.apply then
    local w, h = self.stamp:size()
    canvas:blend(self.cursor.x - (w * 0.5), self.cursor.y - (h * 0.5), self.stamp, "add", self.background)
  end

  canvas:copy(self.background)

  canvas:push()
    canvas:transparent(255, true)
    canvas:sprite(self.position.x, self.position.y, self.bank, 0)
    canvas:write(0, 0, self.font, string.format("%.1f FPS", System.fps()))
  canvas:pop()

  canvas:square("fill", self.cursor.x - 4, self.cursor.y - 4, 8, 0)
end

return Main
