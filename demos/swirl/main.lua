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

local Canvas = require("tofu.graphics").Canvas
local Display = require("tofu.graphics").Display
local Font = require("tofu.graphics").Font
local Class = require("tofu.util").Class
local System = require("tofu.core").System

local function distance(x0, y0, x1, y1)
  local dx = x0 - x1
  local dy = y0 - y1
  return math.sqrt((dx * dx) + (dy * dy))
end

local function square(canvas, x, y, s, r, g, b)
  local index = Display.color_to_index(r * 255.0, g * 255.0, b * 255.0)
  canvas:square("fill", x, y, s, index)
  --canvas:square("line", x, y, s, 0)
end

local Main = Class.define()

function Main:__ctor()
  Display.palette("pico-8")

  local canvas = Canvas.default()
  canvas:transparent(0, false)

  local width, height = canvas:size()
  self.max_x = width - 1
  self.max_y = height - 1
  self.center_x = width / 2
  self.center_y = height / 2
  self.max_distance = distance(0, 0, self.center_x, self.center_y)

  self.font = Font.default(0, 15)
end

function Main:input()
end

function Main:update(_)
end

function Main:render(_)
  local canvas = Canvas:default()
  canvas:clear()

  local mx, my = self.max_x, self.max_y
  local md = self.max_distance
  local cx, cy = self.center_x, self.center_y

  local factor = 0.50
  local mxf, myf = mx * factor, my * factor
  local mdf = md * factor

  local t = System.time()

  for y = 0, my, 5 do
    for x = 0, mx, 5 do
      local d = distance(x, y, cx, cy)
      local r = 1.0 - d / md

      local angle = t + r * math.pi -- Angle increase as we reach the center.
      local c, s = math.cos(angle), math.sin(angle)
      local rx = x - cx
      local ry = y - cy
      rx, ry = c * rx - s * ry, s * rx + c * ry
      rx = rx + cx
      ry = ry + cy

      local d2 = distance(rx, ry, cx, cy) -- Compute color according to the position in the original.
      local r2 = 1.0 - d2 / mdf -- We should normalized differently, however.
      square(canvas, x, y, 5, rx / mxf, ry / myf, r2)
    end
  end

  self.font:write(string.format("FPS: %d", System.fps()), 0, 0)
end

return Main
