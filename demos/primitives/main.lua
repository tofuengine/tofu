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
local Canvas = require("tofu.graphics").Canvas
local Display = require("tofu.graphics").Display
local Font = require("tofu.graphics").Font

local Main = Class.define()

local PALETTE = {
    0xFF000000, 0xFF240000, 0xFF480000, 0xFF6D0000,
    0xFF910000, 0xFFB60000, 0xFFDA0000, 0xFFFF0000,
    0xFFFF3F00, 0xFFFF7F00, 0xFFFFBF00, 0xFFFFFF00,
    0xFFFFFF3F, 0xFFFFFF7F, 0xFFFFFFBF, 0xFFFFFFFF
  }

function Main:__ctor()
  Display.palette(PALETTE) -- "arne-16")
  Display.palette("arne-16")

  local canvas = Canvas.default()
  canvas:color(3)

  self.font = Font.default(0, 1)
  self.mode = 0
end

function Main:input()
  if Input.is_pressed("start") then
    System.quit()
  elseif Input.is_pressed("right") then
    self.mode = (self.mode % 10) + 1
  elseif Input.is_pressed("left") then
    self.mode = ((self.mode + 8) % 10) + 1
  end
end

function Main:update(_) -- delta_time
end

function Main:render(_) -- ratio
  local canvas = Canvas.default()
  canvas:clear()

  local width, height = canvas:size()

  if self.mode == 0 then
    local cx, cy = 8, 32
    for r = 0, 12 do
      canvas:circle("fill", cx, cy, r)
      canvas:circle("line", cx, cy + 64, r)
      cx = cx + 2 * r + 8
    end

    canvas:polyline({ 64, 64, 64, 128, 128, 128 })

    local x0 = (math.random() * width * 2) - width * 0.5
    local y0 = (math.random() * width * 2) - width * 0.5
    local x1 = (math.random() * width * 2) - width * 0.5
    local y1 = (math.random() * width * 2) - width * 0.5
    canvas:line(x0, y0, x1, y1)
  elseif self.mode == 1 then
    local dx = math.cos(System.time()) * 32
    local dy = math.sin(System.time()) * 32
    canvas:circle("fill", 128, 64, 32, 1)
    canvas:line(128, 64, 128 + dx, 64 + dy, 2)
  elseif self.mode == 2 then
    canvas:triangle("fill", 5, 50, 5, 150, 150, 150, 1)
    canvas:triangle("fill", 5, 50, 150, 50, 150, 150, 3)
  elseif self.mode == 3 then
    local x0 = ((math.cos(System.time() * 0.125) + 1.0) * 0.5) * width
    local y0 = ((math.cos(System.time() * 0.342) + 1.0) * 0.5) * height
    local x1 = ((math.sin(System.time() * 0.184) + 1.0) * 0.5) * width
    local y1 = ((math.sin(System.time() * 0.223) + 1.0) * 0.5) * height
    local x2 = ((math.cos(System.time() * 0.832) + 1.0) * 0.5) * width
    local y2 = ((math.sin(System.time() * 0.123) + 1.0) * 0.5) * height
    canvas:triangle("fill", x0, y0, x1, y1, x2, y2, 2)
    canvas:triangle("line", x0, y0, x1, y1, x2, y2, 7)
  elseif self.mode == 4 then
    local x = ((math.cos(System.time() * 0.125) + 1.0) * 0.5) * width
    local y = ((math.cos(System.time() * 0.342) + 1.0) * 0.5) * height
    canvas:square("fill", x, y, 75, 2)
    canvas:square("line", 96, 96, 64, 2)
  elseif self.mode == 5 then
    local cx = width * 0.5
    local cy = height * 0.5
    canvas:circle("fill", cx, cy, 50, 3)
    canvas:circle("line", cx, cy, 50, 4)
  elseif self.mode == 6 then
    local cx = width * 0.5
    local cy = height * 0.5
    canvas:circle("line", cx, cy, 50, 4)
    canvas:circle("fill", cx, cy, 50, 3)
  elseif self.mode == 7 then
    local cx = ((math.cos(System.time() * 0.125) + 1.0) * 0.5) * width
    local cy = ((math.cos(System.time() * 0.342) + 1.0) * 0.5) * height
    local r = ((math.sin(System.time() * 0.184) + 1.0) * 0.5) * 63 + 1
    canvas:circle("fill", cx, cy, r, 6)
  elseif self.mode == 8 then
    local cx = ((math.cos(System.time() * 0.125) + 1.0) * 0.5) * width
    local cy = ((math.cos(System.time() * 0.342) + 1.0) * 0.5) * height
    local r = ((math.sin(System.time() * 0.184) + 1.0) * 0.5) * 63 + 1
    canvas:circle("line", cx, cy, r, 7)
  elseif self.mode == 9 then
    local colors = { 13, 11, 9, 7, 5, 3, 1 }
    local y = (math.sin(System.time()) + 1.0) * 0.5 * height
    canvas:hline(0, y, width - 1, 15)
    for i, c in ipairs(colors) do
      canvas:hline(0, y - i, width - 1, c)
      canvas:hline(0, y + i, width - 1, c)
    end
  elseif self.mode == 10 then
    canvas:point(4, 4, 1)
    canvas:line(8, 8, 32, 32, 2)
    canvas:rectangle("line", 4, 23, 8, 8, 3)
    canvas:triangle("line", 150, 150, 50, 250, 250, 250, 3)
    canvas:rectangle("fill", 4, 12, 8, 8, 3)
  end

  self.font:write(string.format("FPS: %d", System.fps()), 0, 0)
  self.font:write(self.font:align(string.format("mode: %d", self.mode), width, 0, "right"))
end

return Main
