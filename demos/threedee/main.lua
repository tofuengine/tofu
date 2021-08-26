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
local Player = require("lib.player")

local DEBUG <const> = false
local COUNT <const> = 250

local Main = Class.define()

function Main:__ctor()
  local palette = Palette.new("famicube")
  Display.palette(palette)

  local canvas = Canvas.default()
  local width, height = canvas:size()
  canvas:transparent({ [0] = false, [63] = true })

  self.near = 1
  self.far = 1000
  self.field_of_view = math.pi / 2

  self.player = Player.new()
  self.camera = Camera.new(self.field_of_view, width, height, self.near, self.far)
  self.bank = Bank.new(Canvas.new("assets/sheet.png", 63), 40, 158)
  self.terrain = Canvas.new("assets/terrain.png", 63)
  self.font = Font.default(63, 11)

  self.entities = {}
  for _ = 1, COUNT do
    table.insert(self.entities, { x = math.random(-5000, 5000), y = 0, z = math.random(0, 5000) })
    -- Spawn at ground level.
  end
  for i = 1, self.far * 10 do
    table.insert(self.entities, { x = -250, y = 0, z = i * 100 })
    table.insert(self.entities, { x =  250, y = 0, z = i * 100 })
  end
  Arrays.sort(self.entities, function(a, b) -- Farthest first.
      return a.z < b.z
    end)

  self.running = true
end

function Main:process()
  local update = false

  self.player:process()

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

  self.player:update(delta_time)
  self.camera:move(self.player:position())
end

local function _distance_to_scale(d)
  local r = (1 - d) + 0.05
  return r * 2
end

----[[
local function _render_terrain(camera)
  local x <const> = camera.x
  local far <const> = camera.far + camera.z
  local near <const> = camera.near + camera.z

  local _, _, _, _, sy0 = camera:project(x, 0.0, far) -- Straight forward, on ground level.
  local y0 = math.tointeger(sy0 + 0.5) - 1

  local program = Program.gradient(59, {
    { 0, 0x00, 0xBE, 0xDA },
    { y0 * 0.75, 0x8F, 0xB6, 0xBD },
    { y0, 0xC2, 0xA3, 0xA0 }
  })

  for z = far, near, -1  do
    local i <const> = math.tointeger(z / 125)
    local c <const> = i % 2 == 0 and 28 or 42

    local _, _, _, _, sy = camera:project(x, 0.0, z) -- Straight forward, on ground level.

    local y = math.tointeger(sy + 0.5)
    if y0 ~= y then
      program:wait(0, y)
      program:shift(59, c)
      y0 = y
    end
  end

  return program
end
--]]--
--[[
local function _render_terrain(canvas, camera)
  local width, _ = canvas:size()

  local last_y = camera.ground[camera.far]

  for z = camera.far, camera.near, -1 do
    local i = math.tointeger((camera.z + z) / 125)
    local c = i % 2 == 0 and 28 or 42

    local sy = camera.ground[z]
      print(">>>" .. sy .. " | " .. z)

    local y1 = math.tointeger(sy + 0.5)
    local y0 = last_y
    canvas:rectangle('fill', 0, y0, width, y1 - y0 + 1, c)
    last_y = y1
  end
end
--]]--

function Main:_draw_entity(canvas, camera, entity)
  local x, y, z = entity.x, entity.y, entity.z

  if camera:is_too_far(x, y, z) then
    return
  end

  local px, py, pz, sx, sy = camera:project(x, y, z)

  local scale = _distance_to_scale(pz)
  local w, h = self.bank:size(0, scale, scale)
  local xx, yy = sx - w * 0.5, sy - h * 1.0
  if DEBUG then
    canvas:rectangle('line', xx, yy, w, h, 15)
  end
  self.bank:blit(canvas, xx, yy, 0, scale, scale) -- Align the bottom center.
  if DEBUG then
    self.font:write(canvas, sx, sy - 8,
      string.format("%.3f %.3f %.3f", x, y, z), "center", "middle")
    self.font:write(canvas, sx, sy,
      string.format("%.3f %.3f %.3f", px, py, pz), "center", "middle")
  end
end

function Main:render(_)
  local canvas = Canvas.default()
  canvas:clear(59)

  local camera = self.camera

  local program = _render_terrain(camera)
  Display.program(program)

  for _, entity in reverse_ipairs(self.entities) do
    self:_draw_entity(canvas, camera, entity)
  end

  self.font:write(canvas, 0, 0, string.format("FPS: %d", System.fps()))
  self.font:write(canvas, 0, 10, string.format("%.3f %.3f %.3f", self.field_of_view, self.near, self.far))
end

return Main
