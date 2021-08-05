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
local Body = require("tofu.physics").Body
local World = require("tofu.physics").World

local Bunny = require("lib.bunny")

local INITIAL_BUNNIES = 1
local LITTER_SIZE = 5
local MAX_BUNNIES = 32768

local Main = Class.define()

function Main:__ctor()
  Display.palette(Palette.new("pico-8"))

  World.gravity(0.0, 200.0)

  local canvas = Canvas.default()
  canvas:transparent({ ["0"] = false, ["11"] = true })
  canvas:background(0)

  local width, height = canvas:size()

  self.bunnies = {}
  self.bank = Bank.new(Canvas.new("assets/bunnies.png", 11), "assets/bunnies.sheet")
  self.font = Font.default(11, 6)

  local left = Body.new()
  left:shape("box", 1, height)
  left:type("kinematic")
  left:position(0, height * 0.5)
  left:elasticity(1.0)
  self.left = left

  local bottom = Body.new()
  bottom:shape("box", width, 1)
  bottom:type("kinematic")
  bottom:position(width * 0.5, height)
  bottom:elasticity(1.0)
  self.bottom = bottom

  local right = Body.new()
  right:shape("box", 1, height)
  right:type("kinematic")
  right:position(width, height * 0.5)
  right:density(1.0)
  self.right = right

  for _ = 1, INITIAL_BUNNIES do
    table.insert(self.bunnies, Bunny.new(self.font, self.bank))
  end
end

function Main:process()
  if Input.is_pressed("start") then
    for _ = 1, LITTER_SIZE do
      table.insert(self.bunnies, Bunny.new(self.font, self.bank))
    end
    if #self.bunnies >= MAX_BUNNIES then
      System.quit()
    end
  elseif Input.is_pressed("select") then
    self.bunnies = {}
  end
end

function Main:update(delta_time)
  for _, bunny in ipairs(self.bunnies) do
    bunny:update(delta_time)
  end
end

function Main:render(_)
  local canvas = Canvas.default()
  local width, _ = canvas:size()
  canvas:clear()

  for _, bunny in ipairs(self.bunnies) do
    bunny:render(canvas)
  end

  self.font:write(canvas, 0, 0, string.format("FPS: %d", System.fps()))
  self.font:write(canvas, width, 0, string.format("#%d bunnies", #self.bunnies), "right")
end

return Main
