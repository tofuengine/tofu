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

local MovingBunny = require("lib.moving_bunny")
local StaticBunny = require("lib.static_bunny")

local INITIAL_BUNNIES = 15000
local LITTER_SIZE = 250
local MAX_BUNNIES = 32768

local Main = Class.define()

function Main:__ctor()
  Display.palette("pico-8")

  local canvas = Canvas.default()
  canvas:transparent({ ["0"] = false, ["11"] = true })
  canvas:background(0)

  self.bunnies = {}
  self.bank = Bank.new(canvas, Canvas.new("assets/bunnies.png"), "assets/bunnies.sheet")
  self.batch = Batch.new(self.bank, 5000)
  self.font = Font.default(canvas, 11, 6)
  self.speed = 1.0
  self.running = true
  self.static = true
  self.batched = false

  local Bunny = self.static and StaticBunny or MovingBunny
  for _ = 1, INITIAL_BUNNIES do
    table.insert(self.bunnies, Bunny.new(self.bank, self.batch))
  end
end

function Main:input()
  if Input.is_pressed("start") then
    local Bunny = self.static and StaticBunny or MovingBunny
    for _ = 1, LITTER_SIZE do
      table.insert(self.bunnies, Bunny.new(self.bank, self.batch))
    end
    if #self.bunnies >= MAX_BUNNIES then
      System.quit()
    end
  elseif Input.is_pressed("left") then
    self.speed = self.speed * 0.5
  elseif Input.is_pressed("right") then
    self.speed = self.speed * 2.0
  elseif Input.is_pressed("down") then
    self.speed = 1.0
  elseif Input.is_pressed("select") then
    self.bunnies = {}
  elseif Input.is_pressed("x") then
    self.static = not self.static
  elseif Input.is_pressed("y") then
    self.running = not self.running
  end
end

function Main:update(delta_time)
  if not self.running then
    return
  end

  self.batch:clear()

  for _, bunny in pairs(self.bunnies) do
    bunny:update(delta_time * self.speed)
  end
end

function Main:render(_)
  local canvas = Canvas.default()
  local width, _ = canvas:size()
  canvas:clear()

  self.batch:blit()

  self.font:write(string.format("FPS: %d", System.fps()), 0, 0)
  self.font:write(self.font:align(string.format("#%d bunnies", #self.bunnies), width, 0, "right"))
end

return Main