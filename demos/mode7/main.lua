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

local System = require("tofu.core").System
local Surface = require("tofu.graphics").Surface
local Canvas = require("tofu.graphics").Canvas
local Font = require("tofu.graphics").Font
local Input = require("tofu.events").Input
local Class = require("tofu.util").Class

local Main = Class.define()

--[[
local function build_table(factor) -- 0.4
  local entries = {}

  for i = 1, Canvas.height() do
    local angle = (i / Canvas.height()) * math.pi
    local sx = (1.0 - math.sin(angle)) * factor + 1.0

    local entry = { i - 1, sx, 0.0, 0.0, sx } -- Y A B C D
    table.insert(entries, entry)
  end

  return entries
end
]]--

local function build_table(angle, elevation)
  local cos, sin = math.cos(angle), math.sin(angle)
  local a, b = cos, sin
  local c, d = -sin, cos

  local entries = {}

  for scan_line = 1, Canvas.height() do
    local yc = scan_line
    local p = elevation / yc
    entries[scan_line] = {
        a = a * p,
        b = b * p,
        c = c * p,
        d = d * p,
      }
  end

  return entries
end

function Main:__ctor()
  Canvas.palette("6-bit-rgb")
  Canvas.background(0)

--  self.surface = Surface.new("assets/world.png")
  self.surface = Surface.new("assets/road.png")
  self.font = Font.default(0, 63)
  self.running = true

  self.x = 0
  self.y = 0
  self.angle = 0
  self.speed = 0.0
  self.elevation = 48

  self.surface:matrix(1, 0, 0, 1, Canvas.width() * 0.5, Canvas.height() * 0.5)
  self.surface:table(build_table(math.pi * 0.5 - self.angle, self.elevation))
end

function Main:input()
  local recompute = false

  if Input.is_pressed("select") then
    self.speed = 1.0
  elseif Input.is_pressed("start") then
    self.running = not self.running
  elseif Input.is_pressed("y") then
    self.elevation = self.elevation + 8.0
    recompute = true
  elseif Input.is_pressed("x") then
    self.elevation = self.elevation - 8.0
    recompute = true
  elseif Input.is_pressed("a") then -- STRAFE
    local a = self.angle + math.pi * 0.5
    self.x = self.x + math.cos(a) * 8
    self.y = self.y + math.sin(a) * 8
  elseif Input.is_pressed("b") then -- STRAFE
    local a = self.angle + math.pi * 0.5
    self.x = self.x - math.cos(a) * 8
    self.y = self.y - math.sin(a) * 8
  elseif Input.is_pressed("up") then
    self.speed = self.speed + 16.0
  elseif Input.is_pressed("down") then
    self.speed = self.speed - 16.0
  elseif Input.is_pressed("left") then
    self.angle = self.angle - math.pi * 0.05
    recompute = true
  elseif Input.is_pressed("right") then
    self.angle = self.angle + math.pi * 0.05
    recompute = true
  end

  if recompute then
    self.surface:table(build_table(math.pi * 0.5 - self.angle, self.elevation))
end
end

function Main:update(delta_time)
  if not self.running then
    return
  end

  local cos, sin = math.cos(self.angle), math.sin(self.angle)
  self.x = self.x + (cos * self.speed * delta_time)
  self.y = self.y + (sin * self.speed * delta_time)
  self.surface:offset(-self.x, -self.y)
end

function Main:render(_)
  Canvas.clear()

  Canvas.rectangle("fill", 0, 0, Canvas.width(), Canvas.height() * 0.25, 21)
  self.surface:xform(0, Canvas.height() * 0.25)

  local cx, cy = Canvas.width() * 0.5, Canvas.height() * 0.5
  Canvas.line(cx, cy, cx + math.cos(self.angle) * 10, cy + math.sin(self.angle) * 10, 31)

  Canvas.line(cx, cy, cx + math.cos(math.pi * 0.5 - self.angle) * 10,
              cy + math.sin(math.pi * 0.5 - self.angle) * 10, 47)

  self.font:write(string.format("FPS: %d", System.fps()), 0, 0)
end

return Main