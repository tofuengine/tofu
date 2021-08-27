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

local function _build_table(palette, levels, target, excluded)
  local tr, tg, tb = table.unpack(target)
  local colors = palette:colors()
  local lut = {}
  for i = 0, levels - 1 do
    local shifting = {}
    local ratio = i / (levels - 1)
    for j, color in ipairs(colors) do
      local index = j - 1
      if Arrays.index_of(excluded, index) then
        shifting[index] = index
      else
        local ar, ag, ab = table.unpack(color)
        local r, g, b = Palette.mix(ar, ag, ab, tr, tg, tb, ratio)
        shifting[index] = palette:match(r, g, b)
      end
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

  self.lut = _build_table(palette, 16, { 0x8F, 0xB6, 0xBD }, { 63 })

  self.entities = {}
  for i = 1, self.far * 10 do
    table.insert(self.entities, { x = -100, y = 0, z = i * 100, anchor_x = 0.5, anchor_y = 0.95 })
    table.insert(self.entities, { x =  100, y = 0, z = i * 100, anchor_x = 0.5, anchor_y = 0.95 })
  end
  table.sort(self.entities, function(a, b) -- Farthest first.
      return a.z > b.z
    end)

  self.visibles = {}

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

function Main:update(delta_time)
  if not self.running then
    return
  end

  local player = self.player
  local camera = self.camera

  player:update(delta_time)
  camera:move(player:position())

  -- Rebuild sky and ground (copperlist) program.
  local program = _render_terrain(camera, 59)
  Display.program(program)

  -- Scan and update entries, keeping only the visible ones.
  local visibles = {}
  for _, entity in ipairs(self.entities) do
    local is_visible = self:_update_entity(camera, entity)
    if is_visible then
      table.insert(visibles, entity)
    end
  end

  -- Sort by `fog_depth` and `pz` to reduce the amount of `Canvas.shift()` calls.
  table.sort(visibles, function(a, b) -- Farthest first.
      return a.fog_depth > b.fog_depth or a.z > b.z
    end)

  self.visibles = visibles
end

local function _distance_to_scale(d)
  local r = (1 - d) + 0.05
  return r * 2
end

local function _to_fog_level(pz, threshold, levels)
  if pz >= threshold then
    local ratio = (pz - threshold) / (1.0 - threshold)
    return math.tointeger(ratio * (levels - 1))
  else
    return 0
  end
end

function Main:_update_entity(camera, entity)
  local x, y, z = entity.x, entity.y, entity.z

  if camera:is_too_far(x, y, z) then
    return false
  end

  local px, py, pz, sx, sy = camera:project(x, y, z)

  if camera:is_culled(pz) then
    return false
  end

  entity.fog_depth = _to_fog_level(pz, 0.60, 16)

  local scale = _distance_to_scale(pz)
  local w, h = self.bank:size(0, scale, scale)

  entity.scale = scale
  entity.width = w
  entity.height = h
  entity.px = px
  entity.py = py
  entity.pz = pz
  entity.sx = sx - w * entity.anchor_x
  entity.sy = sy - h * entity.anchor_y

  return true
end

function Main:_draw_entity(canvas, entity)
  if DEBUG then
    canvas:rectangle('line', entity.sx, entity.y, entity.width, entity.height, 15)
  end
  self.bank:blit(canvas, entity.sx, entity.sy, 0, entity.scale, entity.scale)
  if DEBUG then
    self.font:write(canvas, entity.sx, entity.sy - 8,
      string.format("%.3f %.3f %.3f", entity.x, entity.y, entity.z), "center", "middle")
    self.font:write(canvas, entity.sx, entity.sy,
      string.format("%.3f %.3f %.3f", entity.px, entity.py, entity.pz), "center", "middle")
  end
end

function Main:render(_)
  local canvas = Canvas.default()
  canvas:clear(59)

  canvas:push()
  local fog_depth = nil
  for _, entity in ipairs(self.visibles) do
    if fog_depth ~= entity.fog_depth then -- Minimize palette shifting calls.
      fog_depth = entity.fog_depth
      canvas:shift(self.lut[fog_depth])
    end
    self:_draw_entity(canvas, entity, self.lut)
  end
  canvas:pop()

  self.font:write(canvas, 0, 0, string.format("FPS: %d", System.fps()))
  self.font:write(canvas, 0, 10, string.format("%.3f %.3f %.3f", self.field_of_view, self.near, self.far))
end

return Main
