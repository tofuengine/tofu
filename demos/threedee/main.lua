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
local Math = require("tofu.core").Math
local System = require("tofu.core").System
local Input = require("tofu.events").Input
local Bank = require("tofu.graphics").Bank
local Canvas = require("tofu.graphics").Canvas
local Display = require("tofu.graphics").Display
local Palette = require("tofu.graphics").Palette
local Font = require("tofu.graphics").Font

local Camera = require("lib.camera")

local Main = Class.define()

local points = {
  {    0,  250, 1010 },
  { -250, -250, 1010 },
  {  250, -250, 1010 },
  -- { -100, -100, 100 },
  -- {  100, -100, 100 },
  -- {  100,  100, 100 },
  -- { -100,  100, 100 },
  -- { -100, -100, 300 },
  -- {  100, -100, 300 },
  -- {  100,  100, 300 },
  -- { -100,  100, 300 },
}

function Main:__ctor()
  Display.palette(Palette.new("famicube"))

  local canvas = Canvas.default()
  local width, height = canvas:size()
  canvas:transparent({ [0] = false, [63] = true })

  self.near = 1
  self.far = 1000
  self.field_of_view = math.pi / 2

  self.bank = Bank.new(Canvas.new("assets/sheet.png", 63), 138, 138)
  self.camera = Camera.new(self.field_of_view, width, height, self.near, self.far)
  self.font = Font.default(0, 11)
  self.easing = Math.tweener("quadratic_out")
  self.wave = Math.wave("triangle", 5.0, 500.0)
end

function Main:process()
  local update = false

  if Input.is_pressed("down") then
    self.near = math.max(self.near - 10, 1)
    update = true
  elseif Input.is_pressed("up") then
    self.near = math.min(self.near + 10, self.far)
    update = true
  elseif Input.is_pressed("left") then
    self.far = math.max(self.far - 10, self.near)
    update = true
  elseif Input.is_pressed("right") then
    self.far = math.min(self.far + 10, 1000)
    update = true
  elseif Input.is_pressed("y") then
    self.field_of_view = math.min(self.field_of_view + math.pi / 15, math.pi)
    update = true
  elseif Input.is_pressed("x") then
    self.field_of_view = math.max(self.field_of_view - math.pi / 15, 0)
    update = true
  end

  if update then
    self.camera:set_field_of_view(self.field_of_view)
    self.camera:set_clipping_planes(self.near, self.far)
  end
end

function Main:update(_)
end

function Main:render(_)
  local canvas = Canvas.default()
  canvas:clear()

  local t = 0 --System.time()

  local camera = self.camera
  for _, point in ipairs(points) do
    local x, y, z = table.unpack(point)
    z = z - (self.wave(t) + 500)
    local c, s = camera:project(x, y, z)
    if s then
      local cx, cy, cz = table.unpack(c)
      local sx, sy = table.unpack(s)
      local sz = 1.0 - self.easing(cz)
      local scale = sz * 8
      self.bank:blit(canvas, sx, sy, 0, scale, scale, 0)
--      canvas:square('fill', dx, dy, scale, 32)
--      self.font:write(canvas, dx, dy, string.format("%.3f %.3f %.3f", x, y, z), "center", "middle")
    end
  end

  self.font:write(canvas, 0, 0, string.format("FPS: %d", System.fps()))
  self.font:write(canvas, 0, 10, string.format("%.3f %.3f %.3f", self.field_of_view, self.near, self.far))
end

return Main
