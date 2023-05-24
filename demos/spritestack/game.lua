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
local Math = require("tofu.core.math")
local System = require("tofu.core.system")
local Controller = require("tofu.input.controller")
local Tweener = require("tofu.generators.tweener")
local Bank = require("tofu.graphics.bank")
local Canvas = require("tofu.graphics.canvas")
local Display = require("tofu.graphics.display")
local Image = require("tofu.graphics.image")
local Palette = require("tofu.graphics.palette")
local Font = require("tofu.graphics.font")

local Sprite = require("lib/sprite")

local CHUNK_SIZE <const> = 1

local TORQUE <const> = 0.25
local THROTTLE <const> = 50.0
local BRAKE <const> = 25.0

local Game = Class.define()

function Game:__ctor()
  local palette = Palette.default("pico-8-ext")
  Display.palette(palette)

  self.palette = palette
  self.bank = Bank.new(Image.new("assets/images/racing-car-tiny-red.png", 0), 16, 16)
--  self.bank = Bank.new(Image.new("assets/images/racing-car-small-red.png", 0), 32, 32)
  self.font = Font.default(0, 31)
  self.tweener = Tweener.new("sine-out")

  self.sprites = {}

  self.angle = 0

  self.force = 0
  self.torque = 0
  self.force_life = 0
  self.torque_life = 0
end

-- PHYSICS
-- http://www.iforce2d.net/b2dtut/top-down-car
-- file:///C:/Users/mlizza/Downloads/[Andrew_Kirmse]_Game_Programming_Gems_4(z-lib.org).pdf

-- SUPER SPRINT
-- https://daytona500news.blogspot.com/2012/01/super-sprint-intricate-design-details.html
-- http://samd.site/2020/04/10/sprite-stacking.html
-- https://medium.com/@avsnoopy/beginners-guide-to-sprite-stacking-in-gamemaker
-- -studio-2-and-magica-voxel-part-1-f7a1394569c0
function Game:process()
  self.force = 0
  self.torque = 0

  local controller = Controller.default()
  if controller:is_pressed("start") then
    local cx, cy = Canvas.default():image():center()
    for _ = 1, CHUNK_SIZE do
      local sprite = Sprite.new(self.bank, 15, 11, 1, self.palette)
--      local sprite = Sprite.new(self.bank, 31, 19, 1)
      sprite:move(cx, cy)
      table.insert(self.sprites, sprite)
    end
  end
  if controller:is_down("up") then
    self.force = self.force + THROTTLE
  end
  if controller:is_down("down") then
    self.force = self.force - BRAKE
  end
  if controller:is_down("left") then
    self.torque = self.torque - TORQUE * Math.sign(self.force)
  end
  if controller:is_down("right") then
    self.torque = self.torque + TORQUE * Math.sign(self.force)
  end
  if controller:is_pressed("select") then
    self.sprites = {}
  end
  if controller:is_pressed("y") then
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
  local image = canvas:image()
  local width, _ = image:size()
  image:clear(0)

  for _, sprite in ipairs(self.sprites) do
    sprite:render(canvas)
  end

  canvas:write(0, 0, self.font, string.format("FPS: %d", System.fps()))
  canvas:write(width, 0, self.font, string.format("#%d sprites", #self.sprites), "right")
end

return Game