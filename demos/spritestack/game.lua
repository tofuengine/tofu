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
local Math = require("tofu.core").Math
local System = require("tofu.core").System
local Input = require("tofu.events").Input
local Bank = require("tofu.graphics").Bank
local Canvas = require("tofu.graphics").Canvas
local Display = require("tofu.graphics").Display
local Font = require("tofu.graphics").Font

local Sprite = require("lib.sprite")

local CHUNK_SIZE = 1

local TORQUE = 0.25
local THROTTLE = 50.0
local BRAKE = 25.0

local Game = Class.define()

function Game:__ctor()
  Display.palette("pico-8-ext")

  local canvas = Canvas.default()

  self.bank = Bank.new(canvas, Canvas.new("assets/images/slices.png"), 15, 32)
  self.font = Font.default(canvas, 0, 31)
  self.tweener = Math.tweener("sine_out")

  self.sprites = {}

  self.angle = 0

  self.force = 0
  self.torque = 0
  self.force_life = 0
  self.torque_life = 0
end

-- http://www.iforce2d.net/b2dtut/top-down-car
-- file:///C:/Users/mlizza/Downloads/[Andrew_Kirmse]_Game_Programming_Gems_4(z-lib.org).pdf
function Game:input()
  self.force = 0
  self.torque = 0

  if Input.is_pressed("start") then
    local cx, cy = Canvas.default():center()
    for _ = 1, CHUNK_SIZE do
      local sprite = Sprite.new(self.bank, 0, 13, 1)
      sprite:move(cx, cy)
      table.insert(self.sprites, sprite)
    end
  end
  if Input.is_down("up") then
    self.force = self.force + THROTTLE
  end
  if Input.is_down("down") then
    self.force = self.force - BRAKE
  end
  if Input.is_down("left") then
    self.torque = self.torque - TORQUE * Math.sign(self.force)
  end
  if Input.is_down("right") then
    self.torque = self.torque + TORQUE * Math.sign(self.force)
  end
  if Input.is_pressed("select") then
    self.sprites = {}
  end
  if Input.is_pressed("y") then
    self.running = not self.running
  end
end

function Game:update(delta_time)
  self.force_life = math.abs(self.force) > Math.EPSILON and math.min(self.force_life + delta_time, 1) or 0
  self.torque_life = math.abs(self.torque) > Math.EPSILON and math.min(self.torque_life + delta_time, 1) or 0
  for _, sprite in ipairs(self.sprites) do
      sprite:accelerate(self.tweener(self.force_life) * self.force)
      sprite:rotate(self.tweener(self.torque_life) * self.torque)
      sprite:update(delta_time)
  end
end

function Game:render(_)
  local canvas = Canvas.default()
  local width, _ = canvas:size()
  canvas:clear()

  for _, sprite in ipairs(self.sprites) do
    sprite:render()
  end

  self.font:write(string.format("FPS: %d", System.fps()), 0, 0)
  self.font:write(self.font:align(string.format("#%d sprites", #self.sprites), width, 0, "right"))
end

return Game