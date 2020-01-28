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

local Input = require("tofu.events").Input
local Canvas = require("tofu.graphics").Canvas
local Display = require("tofu.graphics").Display
local Font = require("tofu.graphics").Font
local Class = require("tofu.util").Class
local System = require("tofu.core").System

local function length(x, y)
  return math.sqrt((x * x) + (y * y))
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
  self.fan = false

  self.font = Font.default(0, 15)
end

function Main:input()
  if Input.is_pressed("start") then
    self.fan = not self.fan
  end
end

function Main:update(_)
end

function Main:render(_)
  local canvas = Canvas:default()
  canvas:clear()

  local mx, my = self.max_x, self.max_y

  local t = System.time()

  for y = 0, my, 7 do
    local oy = (y / my) * 2 - 1
    for x = 0, mx, 7 do
      local ox = (x / mx) * 2 - 1

      local d = length(ox, oy)
      local r = 1.0 - d

      local angle = t + r * math.pi -- Angle increase as we reach the center.
      local c, s = math.cos(angle), math.sin(angle)
      local rx, ry = c * ox - s * oy, s * ox + c * oy

      local v = math.min(1.0, length(rx, ry))
      v = 1.0 - v * v -- Tweak to smooth the color change differently.

      if self.fan then
        local rad = math.atan(ry, rx) + math.pi -- Find the octanct of the rotated point to pick the color.
        local deg = math.floor(rad * (180.0 / math.pi)) % 180
        if deg > 3 and deg < 87 then
          square(canvas, x, y, 5, 0.0, 0.5, 1.0)
        elseif deg > 93 and deg < 177 then
          square(canvas, x, y, 5, 0.0, 1.0, 0.0)
        else
          square(canvas, x, y, 5, 0.0, 0.0, 0.0)
        end
      else
        square(canvas, x, y, 5, rx, ry, v)
      end
    end
  end

  self.font:write(string.format("FPS: %d", System.fps()), 0, 0)
end

return Main
