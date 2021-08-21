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
local Font = require("tofu.graphics").Font
local Palette = require("tofu.graphics").Palette
local Program = require("tofu.graphics").Program
local Arrays = require("tofu.util").Arrays

local Camera = require("lib.camera")

local COUNT <const> = 32
local SPEED <const> = 150.0

local Main = Class.define()

function Main:__ctor()
  Display.palette(Palette.new("famicube"))

  local program = Program.gradient(59, {
    { 0, 0x98, 0xdc, 0xff },
    { 96, 0x5b, 0xa8, 0xff },
    { 128, 0x0a, 0x89, 0xff}
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
  self.easing = Math.tweener("quadratic_out")
  self.wave = Math.wave("triangle", 5.0, 500.0)

  self.entities = {}

  self.running = true
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
  elseif Input.is_pressed("b") then
    self.camera.y = math.min(self.camera.y + 5.0, 500.0)
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

local function _fartherst_first(a, b)
  return a.z < b.z
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

  if #self.entities < COUNT then
    table.insert(self.entities, { x = math.random(-1000, 1000), y = 0, z = 1000.0 }) -- Spawn them on ground level.
  end

  for _, entity in ipairs(self.entities) do
    entity.z = entity.z - SPEED * delta_time
  end

  Arrays.sort(self.entities, _fartherst_first)
end

local function _distance(a, b)
  local dx = a.x - b.x
  local dy = a.y - b.y
  local dz = a.z - b.z
  local d = math.sqrt(dx * dx + dy * dy + dz * dz)
  return math.max(math.min(d / 1500.0, 1.0), 0.0)
end

function Main:render(_)
  local canvas = Canvas.default()
  local width, _ = canvas:size()
  canvas:clear(59)

  local camera = self.camera

  local i = 0
  for z = camera.far, camera.near, -50 do
    local _, _, _, _, sy = camera:project(camera.x, 0.0, z) -- Straight forward the camera, on ground level.
    local _, _, _, _, syp1 = camera:project(camera.x, 0.0, z - 50)
    if not sy or not syp1 then
      break
    end
    --print(z .. " " .. sy .. " " .. syp1)
    canvas:rectangle('fill', 0, sy, width, syp1 - sy + 1, i % 2 == 0 and 18 or 22)
    i = i + 1
  end

  for index, entity in reverse_ipairs(self.entities) do
    local _, _, pz, sx, sy = camera:project(entity.x, entity.y, entity.z)
    if not sx or not sy then
      --print(string.format("dead at %.3f, %.3f, %.3f", px, py, pz))
      --print(index .. " " .. #self.entities)
      table.remove(self.entities, index)
    else
      local d = pz or _distance(camera, entity)
      local sz = 1.0 - self.easing(d) -- Scaling factor should depend on camera-to-entity distance!
      local scale = sz * 4
      local w, h = self.bank:size(0, scale, scale)
      self.bank:blit(canvas, sx - w * 0.5, sy - h * 1.0, 0, scale, scale) -- Align the bottom center.
      --canvas:square('fill', sx - 2, sy - 2, 4, 17)
--      self.font:write(canvas, sx, sy,
--        string.format("%.3f %.3f %.3f (%.3f)", px, py, pz, d), "center", "middle")
--      self.font:write(canvas, dx, dy, string.format("%.3f %.3f %.3f", x, y, z), "center", "middle")
    end
  end

  self.font:write(canvas, 0, 0, string.format("FPS: %d", System.fps()))
  self.font:write(canvas, 0, 10, string.format("%.3f %.3f %.3f", self.field_of_view, self.near, self.far))
end

return Main
