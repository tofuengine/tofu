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
local Canvas = require("tofu.graphics").Canvas
local Display = require("tofu.graphics").Display
local Palette = require("tofu.graphics").Palette
local Font = require("tofu.graphics").Font

local function length(x, y)
  return math.sqrt((x * x) + (y * y))
end

local function square(canvas, x, y, s, r, g, b)
--  local index = Display.color_to_index(r * 255.0, g * 255.0, b * 255.0)
  local index = math.floor(math.min(1.0, r * g + b) * 15)
  canvas:square("fill", x, y, s, index)
end

local Main = Class.define()

function Main:__ctor()
  Display.palette(Palette.new("pico-8"))

  local canvas = Canvas.default()
  canvas:transparent(0, false)

  local width, height = canvas:size()
  self.max_x = width - 1
  self.max_y = height - 1
  self.fan = false

  self.font = Font.default(0, 15)

  self.time = 0
end

function Main:input()
  if Input.is_pressed("start") then
    self.fan = not self.fan
  end
end

function Main:update(delta_time)
  if not System.is_active() then
    return
  end
  self.time = self.time + delta_time
end

function Main:render(_)
  local canvas = Canvas:default()
  -- Note that we *don't* clear the canvas on purpose!!!

  local mx, my = self.max_x, self.max_y

  local t = self.time

  for _ = 1, 1000 do
    local x, y = math.random(0, mx), math.random(0, my)
    local ox, oy = (x / mx) * 2 - 1, (y / my) * 2 - 1
    local d = length(ox, oy)
    local r = 1.0 - d

    local angle = t * 3 + r * math.pi -- Angle increase as we reach the center.
    local c, s = math.cos(angle), math.sin(angle)
    local rx, ry = c * ox - s * oy, s * ox + c * oy

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
      local v = math.min(1.0, length(rx, ry))
      v = 1.0 - v * v -- Tweak to smooth the color change differently.

      square(canvas, x, y, 5, rx, ry, v)
    end
  end

  self.font:write(canvas, 0, 0, string.format("FPS: %d", System.fps()))
end

return Main
