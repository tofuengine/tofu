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
local Math = require("tofu.core").Math
local System = require("tofu.core").System
local Input = require("tofu.events").Input
local Canvas = require("tofu.graphics").Canvas
local Display = require("tofu.graphics").Display
local Font = require("tofu.graphics").Font

local SIZE = 4
local RADIUS = SIZE * 0.5

local Main = Class.define()

function Main:__ctor()
  Display.palette("pico-8-ext")

  local canvas = Canvas.default()

  self.font = Font.default(canvas, 0, 15)
  self.factor = 0.75
end

function Main:input()
  if Input.is_pressed("left") then
    self.factor = self.factor - 0.01
  elseif Input.is_pressed("right") then
    self.factor = self.factor + 0.01
  end
end

function Main:update(_)
end

function Main:render(_)
  local canvas = Canvas.default()
  local width, height = canvas:size()
  canvas:clear()

  local x = width * 0.5

  for y = 0, height, SIZE * 2 do
    local coords = {}
    for i = 0, 1 do
      local t = System.time() * 2 + y * self.factor + math.pi * i
      local v = (math.cos(t) * 0.5 + 0.5) * 255
      local cy = y + SIZE + RADIUS
      local cx = x + math.sin(t) * 48 + RADIUS
      table.insert(coords, { x = cx, y = cy, v = v })
    end

    local py = coords[1].y
    local delta_x = coords[1].x < coords[2].x and 1 or -1
    for px = coords[1].x, coords[2].x, delta_x do
      local r = (px - coords[1].x) / (coords[2].x - coords[1].x)
      local v = Math.lerp(coords[1].v, coords[2].v, r)
      local index = Display.color_to_index(v, v, v)
      canvas:point(px, py, index)
    end

    local v1 = coords[1].v
    canvas:circle("fill", coords[1].x, coords[1].y, RADIUS, Display.color_to_index(v1, v1, v1))

    local v2 = coords[2].v
    canvas:circle("fill", coords[2].x, coords[2].y, RADIUS, Display.color_to_index(v2, v2, v2))
  end

  self.font:write(string.format("FPS: %d", System.fps()), 0, 0)
end

return Main
