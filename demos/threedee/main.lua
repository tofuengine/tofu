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

local config = require("config")

local DEBUG <const> = config.debug
local RUMBLE_LENGTH <const> = config.ground.rumble_length

local Main = Class.define()

local function _build_table(palette, levels, target)
  local tr, tg, tb = table.unpack(target)
  local colors = palette:colors()
  local lut = {}
  for i = 0, levels - 1 do
    local shifting = {}
    local ratio = i / (levels - 1)
    for j, color in ipairs(colors) do
      local ar, ag, ab = table.unpack(color)
      local r, g, b = Palette.mix(ar, ag, ab, tr, tg, tb, 1 - ratio)
      shifting[j - 1] = palette:match(r, g, b)
    end
    lut[i] = shifting
  end
  return lut
end

function Main:__ctor()
  local palette = Palette.new(config.display.palette)
  Display.palette(palette)

  local canvas = Canvas.default()
  local width, height = canvas:size()
  canvas:transparent({ [0] = false, [63] = true })

  self.near = config.camera.near
  self.far = config.camera.far
  self.field_of_view = config.camera.field_of_view

  self.player = Player.new()
  self.camera = Camera.new(self.field_of_view, width, height, self.near, self.far)
  self.bank = Bank.new(Canvas.new("assets/sheet.png", 63), 40, 158)
  self.terrain = Canvas.new("assets/terrain.png", 63)
  self.font = Font.default(63, 11)

  self.lut = _build_table(palette, 16, { 0x8F, 0xB6, 0xBD})

  self.entities = {}
  for _ = 1, config.objects.count do
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
    self.far = math.min(self.far + 50.0, 5000.0)
    update = true
  elseif Input.is_pressed("a") then
    self.far = math.max(self.far - 50.0, 0)
    update = true
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

local function _render_terrain(camera, index)
  local x <const> = camera.x
  local far <const> = camera.far + camera.z
  local near <const> = camera.near + camera.z

  local _, _, _, _, sy0 = camera:project(x, 0.0, far) -- Straight forward, on ground level.
  local y0 = math.tointeger(sy0 + 0.5)

  local program = Program.gradient(index, {
    { 0, 0x00, 0xBE, 0xDA },
    { y0 * 0.75, 0x8F, 0xB6, 0xBD },
    { y0 - 1, 0xC2, 0xA3, 0xA0 }
  })

  local toggle = math.tointeger(far / RUMBLE_LENGTH) % 2 == 0 -- Find the initial color.
  program:wait(0, y0)
  program:shift(index, toggle and 28 or 42)

  local less_far <const> = far - math.tointeger(far % RUMBLE_LENGTH) -- Skip to the start of the new rumble.

  for z = less_far, near, -RUMBLE_LENGTH  do
    local _, _, _, _, sy = camera:project(x, 0.0, z) -- Straight forward, on ground level.
    toggle = not toggle
    program:wait(0, math.tointeger(sy + 0.5))
    program:shift(index, toggle and 28 or 42)
  end

  return program
end

function Main:_draw_entity(canvas, camera, entity, lut)
  local x, y, z = entity.x, entity.y, entity.z

  if camera:is_too_far(x, y, z) then
    return
  end

  local px, py, pz, sx, sy = camera:project(x, y, z)

  -- TODO: should fade distant objects.
  if pz >= 0.40 then
    local depth = math.tointeger((1 - (pz - 0.40) / 0.60) * 15)
    canvas:shift(lut[depth])
    canvas:shift(63, 63)
  end

  local scale = _distance_to_scale(pz)
  local w, h = self.bank:size(0, scale, scale)
  local xx, yy = sx - w * 0.5, sy - h * 0.95
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

  local program = _render_terrain(camera, 59)
  Display.program(program)

  canvas:push()
  for _, entity in reverse_ipairs(self.entities) do
    self:_draw_entity(canvas, camera, entity, self.lut)
  end
  canvas:pop()

  self.font:write(canvas, 0, 0, string.format("FPS: %d", System.fps()))
  self.font:write(canvas, 0, 10, string.format("%.3f %.3f %.3f", self.field_of_view, self.near, self.far))
end

return Main
