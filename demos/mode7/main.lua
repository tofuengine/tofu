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
local Font = require("tofu.graphics.font")
local Image = require("tofu.graphics.image")
local Palette = require("tofu.graphics.palette")
local XForm = require("tofu.graphics.xform")

local WRAP_MODES <const> = {
  "repeat", "edge", "border", "mirror-repeat", "mirror-edge", "mirror-border"
}

local Main = Class.define()

function Main:__ctor()
  local palette = Palette.default("famicube")
  Display.palette(palette)

  local canvas = Canvas.default()
  self.background = palette:match(31, 31, 63)

  self.font = Font.default("5x8", 0, palette:match(0, 255, 0))
  self.surface = Image.new("assets/road.png", 0)
  self.xform = XForm.new()
  self.running = true
  self.wrap = 1

  self.x = 0
  self.y = 0
  self.angle = 0
  self.speed = 0.0
  self.elevation = 48

  local image = canvas:image()
  local width, height = image:size()
  self.xform:wrap(WRAP_MODES[self.wrap])
  self.xform:matrix(1, 0, 0, 1, width * 0.5, height * 0.5)
  self.xform:project(height, math.pi * 0.5 - self.angle, self.elevation)
end

function Main:process()
  local recompute = false

  local controller = Controller.default()
  if controller:is_pressed("select") then
    self.speed = 1.0
  elseif controller:is_pressed("start") then
    self.running = not self.running
  elseif controller:is_pressed("y") then
    self.elevation = self.elevation + 4.0
    recompute = true
  elseif controller:is_pressed("x") then
    self.elevation = self.elevation - 4.0
    recompute = true
  elseif controller:is_pressed("a") then -- STRAFE
    local a = self.angle + math.pi * 0.5
    self.x = self.x + math.cos(a) * 8
    self.y = self.y + math.sin(a) * 8
  elseif controller:is_pressed("b") then -- STRAFE
    local a = self.angle + math.pi * 0.5
    self.x = self.x - math.cos(a) * 8
    self.y = self.y - math.sin(a) * 8
  elseif controller:is_pressed("rt") then
    self.wrap = (self.wrap % #WRAP_MODES) + 1
    self.xform:wrap(WRAP_MODES[self.wrap])
  elseif controller:is_pressed("up") then
    self.speed = self.speed + 16.0
  elseif controller:is_pressed("down") then
    self.speed = self.speed - 16.0
  elseif controller:is_pressed("left") then
    self.angle = self.angle - math.pi * 0.05
    recompute = true
  elseif controller:is_pressed("right") then
    self.angle = self.angle + math.pi * 0.05
    recompute = true
  end

  if recompute then
    local canvas = Canvas.default()
    local image = canvas:image()
--    self.xform:table(build_table(canvas, math.pi * 0.5 - self.angle, self.elevation))
    local _, height = image:size()
    self.xform:project(height, math.pi * 0.5 - self.angle, self.elevation)
  end
end

function Main:update(delta_time)
  if not self.running then
    return
  end

  local cos, sin = math.cos(self.angle), math.sin(self.angle)
  self.x = self.x + (cos * self.speed * delta_time)
  self.y = self.y + (sin * self.speed * delta_time)
  self.xform:offset(-self.x, -self.y)
end

function Main:render(_)
  local canvas = Canvas.default()
  local image = canvas:image()
  local width, height = image:size()

  image:clear(self.background)

--  canvas:rectangle("fill", 0, 0, width, height * 0.25, 21)
  canvas:xform(0, height * 0.25, self.surface, self.xform)

  local cx, cy = width * 0.5, height * 0.5
  canvas:line(cx, cy, cx + math.cos(self.angle) * 10, cy + math.sin(self.angle) * 10, 31)

  canvas:line(cx, cy, cx + math.cos(math.pi * 0.5 - self.angle) * 10,
              cy + math.sin(math.pi * 0.5 - self.angle) * 10, 47)

  canvas:write(0, 0, self.font, string.format("%d FPS", System.fps()))
  canvas:write(width, 0, self.font, string.format("mode: %s", WRAP_MODES[self.wrap]), "right", "top")
  canvas:write(width, height, self.font, string.format("mem: %.3f MiB", System.heap("m")), "right", "bottom")
end

return Main
