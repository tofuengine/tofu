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
local Copperlist = require("tofu.graphics").Copperlist
local Display = require("tofu.graphics").Display
local Font = require("tofu.graphics").Font

local Main = Class.define()

function Main:__ctor()
  Display.palette("pico-8-ext")

  local canvas = Canvas.default()
  local width, height = canvas:size()
  canvas:transparent({ ["0"] = false, ["22"] = true })

  self.font = Font.default(canvas, 0, 11)
  self.bank = Bank.new(canvas, Canvas.new("assets/sprites.png"), 16, 16)
  self.big_font = Font.default(canvas, "32x64", 1, 31)
  self.running = true
  self.time = 0
  self.dx = 0
  self.dy = 0
  self.x = width * 0.5
  self.y = height * 0.25
end

function Main:input()
  if Input.is_pressed("y") then
    self.running = not self.running
  end

  self.dx = 0
  self.dy = 0
  if Input.is_down("up") then
    self.dy = self.dy - 1
  end
  if Input.is_down("down") then
    self.dy = self.dy + 1
  end
  if Input.is_down("left") then
    self.dx = self.dx - 1
  end
  if Input.is_down("right") then
    self.dx = self.dx + 1
  end
end

function Main:update(delta_time)
  if not self.running then
    return
  end

  self.time = self.time + delta_time

  self.x = self.x + self.dx * 64 * delta_time
  self.y = self.y + self.dy * 64 * delta_time

  local canvas = Canvas.default()
  local width, height = canvas:size()

  local t = self.time
  local y = math.sin(t * 2.5) * height * 0.125 + height * 0.25

  local copperlist = Copperlist.new()
  copperlist:wait(0, 0)
  copperlist:color(0, 0x00, 0x00, 0x00)
  copperlist:color(1, 0x00, 0x00, 0x00)
  copperlist:color(31, 0x00, 0x00, 0x00)
  for i = 15, 0, -1 do
    local v = 0x11 * (15 - i)
    copperlist:wait(0, y - i)
    copperlist:color(0, v, 0x00, 0x00)
    copperlist:color(1, v, 0x00, v)
    copperlist:color(31, v, v, 0x00)
  end
  for i = 0, 15 do
    local v = 0x11 * (15 - i)
    copperlist:wait(0, y + i)
    copperlist:color(0, v, 0x00, 0x00)
    copperlist:color(1, v, 0x00, v)
    copperlist:color(31, v, v, 0x00)
  end
  copperlist:wait(0, height * 0.5)
  copperlist:color(0, 0x00, 0x11, 0x44)
  copperlist:color(1, 0x00, 0x11, 0x44)
  copperlist:color(31, 0x00, 0x11, 0x44)
  copperlist:modulo(-width * 2)
  for i = math.tointeger(height * 0.5), height - 1 do
    copperlist:wait(0, i)
    copperlist:offset(math.sin(t * 13.0 + i * 0.25) * 1.5)
  end
  Display.copperlist(copperlist)
end

function Main:render(_)
  local canvas = Canvas.default()
  local _, height = canvas:size()

  canvas:clear()

  local t = self.time
  local y = math.sin(t * 0.5) * height * 0.125 + height * 0.25
  self.big_font:write("TOFU ENGINE", 0, y)

  self.bank:blit(12, self.x, self.y, 4, 4, 0)

  self.font:write(string.format("FPS: %d", System.fps()), 0, 0)
end

return Main