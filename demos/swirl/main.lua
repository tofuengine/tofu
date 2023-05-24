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
local System = require("tofu.core.system")
local Controller = require("tofu.input.controller")
local Canvas = require("tofu.graphics.canvas")
local Display = require("tofu.graphics.display")
local Palette = require("tofu.graphics.palette")
local Font = require("tofu.graphics.font")
local Vector = require("tofu.util.vector")

local function square(canvas, x, y, s, r, g, b)
--  local index = Display.color_to_index(r * 255.0, g * 255.0, b * 255.0)
  local index = math.floor(math.min(1.0, r * g + b) * 15)
  canvas:square("fill", x, y, s, index)
end

local Main = Class.define()

function Main:__ctor()
  Display.palette(Palette.default("pico-8"))

  local canvas = Canvas.default()
  canvas:transparent(0, false)

  local image = canvas:image()
  local width, height = image:size()
  self.m = Vector.new(width - 1, height - 1)
  self.c = Vector.new(self.m)
  self.c:scale(0.5)
  self.fan = false

  self.font = Font.default(0, 15)

  self.time = 0
  self.running = true
end

function Main:on_focus_acquired()
  self.running = true
end

function Main:on_focus_lost()
  self.running = false
end

function Main:process()
  local controller = Controller.default()
  if controller:is_pressed("start") then
    self.fan = not self.fan
  end
end

function Main:update(delta_time)
  if not self.running then
    return
  end
  self.time = self.time + delta_time
end

function Main:render(_)
  local canvas = Canvas:default()
  -- Note that we *don't* clear the canvas on purpose!!!

  local m = self.m
  local c = self.c

  local t = self.time

  for _ = 1, 250 do
    local p = Vector.new(math.random(0, m.x), math.random(0, m.y))
    local v = Vector.from_points(c, p)
    v:div(c) -- Normalize and center in [-1, 1]
    local d = 1.0 - v:magnitude()
    local angle = t * 3 + d * math.pi -- Angle increase as we reach the center.
    v:rotate(angle)

    if self.fan then
      local rad = v:angle_to() + math.pi -- Find the octant of the rotated point to pick the color.
      local deg = math.floor(rad * (180.0 / math.pi)) % 180
      if deg > 3 and deg < 87 then
        square(canvas, p.x, p.y, 5, 0.0, 0.5, 1.0)
      elseif deg > 93 and deg < 177 then
        square(canvas, p.x, p.y, 5, 0.0, 1.0, 0.0)
      else
        square(canvas, p.x, p.y, 5, 0.0, 0.0, 0.0)
      end
    else
      local l = math.min(1.0, v:magnitude())
      l = 1.0 - l * l -- Tweak to smooth the color change differently.

      square(canvas, p.x, p.y, 5, v.x, v.y, l)
    end
  end

  canvas:write(0, 0, self.font, string.format("FPS: %d", System.fps()))
end

return Main
