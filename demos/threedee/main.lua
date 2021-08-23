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
local Display = require("tofu.graphics").Display
local Font = require("tofu.graphics").Font
local Palette = require("tofu.graphics").Palette
local Program = require("tofu.graphics").Program
local Arrays = require("tofu.util").Arrays

local Camera = require("lib.camera")

local DEBUG <const> = false
local COUNT <const> = 250
local SPEED <const> = 500.0

local Main = Class.define()

function Main:__ctor()
  local palette = Palette.new("famicube")
  Display.palette(palette)

  local program = Program.gradient(59, {
    { 0, 0x00, 0xBE, 0xDA },
    { 96, 0x8F, 0xB6, 0xBD },
    { 128, 0xC2, 0xA3, 0xA0 }
  })
  Display.program(program)

  local canvas = Canvas.default()
  local width, height = canvas:size()
  canvas:transparent({ [0] = false, [63] = true })

  self.near = 1
  self.far = 1000
  self.field_of_view = math.pi / 2

  self.bank = Bank.new(Canvas.new("assets/sheet.png", 63), 40, 158)
  self.camera = Camera.new(self.field_of_view, width, height, self.near, self.far)
  self.font = Font.default(63, 11)

  self.entities = {}
  for _ = 1, COUNT do
    table.insert(self.entities, { x = math.random(-5000, 5000), y = 0, z = math.random(0, 5000) })
    -- Spawn at ground level.
  end
  Arrays.sort(self.entities, function(a, b) -- Farthers first.
      return a.z < b.z
    end)

  self.running = true
end

function Main:process()
  local update = false

  self.dx = 0
  self.dy = 0
  self.dz = 0
  if Input.is_down("up") then
    self.dz = self.dz + 1
  end
  if Input.is_down("down") then
    self.dz = self.dz - 1
  end
  if Input.is_down("left") then
    self.dx = self.dx - 1
  end
  if Input.is_down("right") then
    self.dx = self.dx + 1
  end

  if Input.is_pressed("y") then
    self.field_of_view = math.min(self.field_of_view + math.pi / 15, math.pi)
    update = true
  elseif Input.is_pressed("x") then
    self.field_of_view = math.max(self.field_of_view - math.pi / 15, 0)
    update = true
  elseif Input.is_pressed("b") then
    self.camera.y = math.min(self.camera.y + 5.0, 1000.0)
  elseif Input.is_pressed("a") then
    self.camera.y = math.max(self.camera.y - 5.0, 0)
  elseif Input.is_pressed("start") then
    self.running = not self.running
  end

  if update then
    self.camera:set_field_of_view(self.field_of_view)
    self.camera:set_clipping_planes(self.near, self.far)
  end
end

local function reverse_ipairs(table)
  return function(a, i)
        i = i - 1
        if i == 0 then
          return nil, nil
        end
        return i, a[i]
    end, table, #table + 1
end

function Main:update(delta_time)
  if not self.running then
    return
  end

  local camera = self.camera
  camera:offset(self.dx * SPEED * delta_time, self.dy * SPEED * delta_time, self.dz * SPEED * delta_time)
end

local function _distance_to_scale(d)
  local r = (1 - d) + 0.05
  return r * 1
end

function Main:render(_)
  local canvas = Canvas.default()
  local width, _ = canvas:size()
  canvas:clear(59)

  local camera = self.camera

  local far = camera.far + camera.z
  local near = camera.near + camera.z
  for z = far, near, -1 do
    local _, _, _, _, sy = camera:project(camera.x, 0.0, z) -- Straight forward the camera, on ground level.
    if sy then
      local i = math.tointeger(z / 125)
      canvas:rectangle('fill', 0, math.tointeger(sy + 0.5), width, 1, i % 2 == 0 and 28 or 42)
    end
  end

  for _, entity in reverse_ipairs(self.entities) do
    local x, y, z = entity.x, entity.y, entity.z
    local px, py, pz, sx, sy = camera:project(x, y, z)
    if sx and sy then
      local scale = _distance_to_scale(pz)
      local w, h = self.bank:size(0, scale, scale)
      self.bank:blit(canvas, sx - w * 0.5, sy - h * 1.0, 0, scale, scale) -- Align the bottom center.
      --canvas:square('fill', sx - 2, sy - 2, 4, 17)
      if DEBUG then
        self.font:write(canvas, sx, sy - 8,
          string.format("%.3f %.3f %.3f", x, y, z), "center", "middle")
        self.font:write(canvas, sx, sy,
          string.format("%.3f %.3f %.3f", px, py, pz), "center", "middle")
      end
    end
  end

  self.font:write(canvas, 0, 0, string.format("FPS: %d", System.fps()))
  self.font:write(canvas, 0, 10, string.format("%.3f %.3f %.3f", self.field_of_view, self.near, self.far))
end

return Main
