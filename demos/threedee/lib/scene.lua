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
local Bank = require("tofu.graphics.bank")
local Image = require("tofu.graphics.image")
local Palette = require("tofu.graphics.palette")
local Arrays = require("tofu.util.arrays")

local config = require("config")

local FOG_COLOR <const> = { 0x8F, 0xB6, 0xBD }
local FOG_LEVELS <const> = config.scene.fog.levels or 8
local FOG_THRESHOLD <const> = config.scene.fog.threshold or 0.5

local DEBUG <const> = config.debug or false

local Scene = Class.define()

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

local TREE_DISTANCE <const> = 80

local function _respawn_entity(camera, entity)
  entity.z = entity.z + camera.far

  if entity.cell_id == 3 then
    entity.x = math.random(-500, 500)
    entity.y = math.random(50, 250)
  elseif entity.cell_id ~= 0 then
    entity.x = math.random(-500, 500)
  end
end

local function _create_entities(camera)
  local entities = {}
  for z = 0, camera.far - TREE_DISTANCE, TREE_DISTANCE do
    table.insert(entities, { kind = "tree", x = -350, y = 0, z = z, cell_id = 0, anchor_x = 0.5, anchor_y = 0.95 })
    table.insert(entities, { kind = "tree", x =  350, y = 0, z = z, cell_id = 0, anchor_x = 0.5, anchor_y = 0.95 })
  end
  for _ = 1, 10 do
    table.insert(entities, {
        kind = "obstacle",
        x = math.random(-500, 500),
        y = 0,
        z = math.random(0, camera.far),
        cell_id = math.random(1, 2),
        anchor_x = 0.5,
        anchor_y = 0.75
      })
  end
  for _ = 1, 10 do
    table.insert(entities, {
        kind = "rock",
        x = math.random(-500, 500),
        y = math.random(50, 250),
        z = math.random(0, camera.far),
        cell_id = 3,
        anchor_x = 0.5,
        anchor_y = 0.5
      })
  end
  table.insert(entities, {
      kind = "floating",
      x = 0,
      y = 500,
      z = camera.far * 0.90,
      cell_id = 1,
      anchor_x = 0.5,
      anchor_y = 0.5,
      infinite = true
    })
  return entities
end

function Scene:__ctor(camera, palette, index)
  self.camera = camera
  self.bank = Bank.new(Image.new("assets/texture.png", index), "assets/texture.sheet")
  self.font = nil

  self.lut = _build_table(palette, FOG_LEVELS, FOG_COLOR, { index })

  self.entities = _create_entities(camera)
  self.visibles = {}
end

local function _distance_to_scale(d)
  -- TODO: fix scale so that nearest objects are bigger.
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

local function _update_entity(camera, entity, bank)
  local x, y, z = entity.x, entity.y, entity.z
  local infinite = entity.infinite

  if camera:is_behind(x, y, z, infinite) then
    if entity.kind == "bullet" then
      return false, true
    else
      _respawn_entity(camera, entity)
    end
  end

  if camera:is_too_far(x, y, z, infinite) then
    return false, false
  end

  local px, py, pz, sx, sy = camera:project(x, y, z, infinite)

  if camera:is_culled(pz) then
    return false, false
  end

  entity.fog_depth = _to_fog_level(pz, FOG_THRESHOLD, FOG_LEVELS)

  local scale = _distance_to_scale(pz)
  local w, h = bank:size(entity.cell_id, scale, scale)

  entity.scale = scale
  entity.width = w
  entity.height = h
  entity.px = px
  entity.py = py
  entity.pz = pz
  entity.sx = sx - w * entity.anchor_x
  entity.sy = sy - h * entity.anchor_y

  return true, false
end

local function z_order(a, b) -- Farthest first.
  return a.fog_depth > b.fog_depth or a.pz > b.pz
end

local PREDICTION <const> = 250

local function _fire_bullet()
  return math.random() >= 0.99
end

function Scene:update(delta_time)
  local camera <const> = self.camera
  local bank <const> = self.bank
  local entities <const> = self.entities

  -- Scan and update entries, keeping only the visible ones.
  for i = 1, #entities do
    local entity = entities[i]
    if entity.kind == "rock" then
      local dx <const> = camera.x - entity.x
      local dy <const> = camera.y - entity.y
      local dz <const> = camera.z + PREDICTION - entity.z -- Assume player will move, anticipate its position.
      if dz < 0 and _fire_bullet() then
        local d <const> = (dx * dx + dy * dy + dz * dz) ^ -0.5
        local vx <const> = dx * d
        local vy <const> = dy * d
        local vz <const> = dz * d
        table.insert(entities, {
          kind = "bullet",
          x = entity.x,
          y = entity.y,
          z = entity.z, -- Just a bit in front of the rock.
          vx = vx * 500,
          vy = vy * 500,
          vz = vz * 250,
          cell_id = 4,
          anchor_x = 0.5,
          anchor_y = 0.5,
          is_moving = true
        })
      end
    end
  end

  local visibles = {}
  local count = 1
  for i = #entities, 1, -1  do -- Reverse loop to permit removal.
    local entity = entities[i]
    if entity.is_moving then
      entity.x = entity.x + entity.vx * delta_time
      entity.y = entity.y + entity.vy * delta_time
      entity.z = entity.z + entity.vz * delta_time
    end
    local is_visible, is_dead = _update_entity(camera, entity, bank)
    if is_visible then
      visibles[count] = entity
      count = count + 1
    elseif is_dead then
      table.remove(entities, i)
    end
  end

  -- Sort by `fog_depth` and `pz` to reduce the amount of `Canvas.shift()` calls.
  table.sort(visibles, z_order)

  self.visibles = visibles
end

local function _draw_entity(canvas, entity, bank, font)
  if DEBUG then
    canvas:rectangle('line', entity.sx, entity.y, entity.width, entity.height, 15)
  end
  canvas:sprite(entity.sx, entity.sy, bank, entity.cell_id, entity.scale, entity.scale)
  if DEBUG then
    canvas:write(entity.sx, entity.sy - 8, font,
      string.format("%.3f %.3f %.3f", entity.x, entity.y, entity.z), "center", "middle")
    canvas:write(entity.sx, entity.sy, font,
      string.format("%.3f %.3f %.3f", entity.px, entity.py, entity.pz), "center", "middle")
  end
end

function Scene:render(canvas)
  local bank <const> = self.bank
  local lut <const> = self.lut
  local entities <const> = self.visibles

  canvas:push()
  local fog_depth = nil
  for i = 1, #entities do
    local entity = entities[i]
    if fog_depth ~= entity.fog_depth then -- Minimize palette shifting calls.
      fog_depth = entity.fog_depth
      canvas:shift(lut[fog_depth])
    end
    _draw_entity(canvas, entity, bank)
  end
  canvas:pop()
end

return Scene
