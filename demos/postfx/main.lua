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
local Bank = require("tofu.graphics.bank")
local Canvas = require("tofu.graphics.canvas")
local Display = require("tofu.graphics.display")
local Font = require("tofu.graphics.font")
local Image = require("tofu.graphics.image")
local Palette = require("tofu.graphics.palette")

local Sprite = require("lib/sprite")

local LITTER_SIZE = 64

local Main = Class.define()

function Main:__ctor()
  Display.palette(Palette.default("nes"))

  local canvas = Canvas.default()
  canvas:transparent({ [0] = false, [13] = true })

  self.sprites = {}
  self.bank = Bank.new(Image.new("assets/images/diamonds.png", 13), 16, 16)
  self.font = Font.default(13, 0)
  self.speed = 1.0
  self.running = true
end

function Main:process()
  local controller = Controller.default()
  if controller:is_pressed("start") then
    for _ = 1, LITTER_SIZE do
      table.insert(self.sprites, Sprite.new(Canvas.default(), self.bank, #self.sprites))
    end
  elseif controller:is_pressed("left") then
    self.speed = self.speed * 0.5
  elseif controller:is_pressed("right") then
    self.speed = self.speed * 2.0
  elseif controller:is_pressed("down") then
    self.speed = 1.0
  elseif controller:is_pressed("select") then
    self.sprites = {}
  elseif controller:is_pressed("y") then
    self.running = not self.running
  end
end

function Main:update(delta_time)
  if not self.running then
    return
  end
  for _, sprite in pairs(self.sprites) do
    sprite:update(delta_time * self.speed)
  end
end

function Main:render(_)
  local canvas = Canvas.default()
  local image = canvas:image()
  local width, _ = image:size()
  image:clear(63)
  for _, sprite in pairs(self.sprites) do
    sprite:render(canvas)
  end
  canvas:write(0, 0, self.font, string.format("FPS: %d", System.fps()))
  canvas:write(width, 0, self.font, string.format("#%d sprites", #self.sprites), "right")
end

return Main
