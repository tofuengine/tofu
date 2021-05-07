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
local Batch = require("tofu.graphics").Batch
local Canvas = require("tofu.graphics").Canvas
local Display = require("tofu.graphics").Display
local Font = require("tofu.graphics").Font
local Palette = require("tofu.graphics").Palette
local Program = require("tofu.graphics").Program
local Vector = require("tofu.util").Vector

local Animation = require("lib.animation")

local WATER_DISPLACEMENT = 1.5

local Main = Class.define()

local function generate_map(screens)
  local map = {}
  for i = 1, 5 do
    local row = {}
    for _ = 1, 15 * screens do
      local cell_id
      if i == 1 then
        cell_id = math.random(3, 8)
      else
        cell_id = i == 2 and 1 or 2
      end
      table.insert(row, cell_id)
    end
    table.insert(map, row)
  end
  return map
end

local function extra_half_brite(palette, target, ratio)
  local p = {}
  for _, color in ipairs(palette) do
    table.insert(p, color)
  end
  local tr, tg, tb = table.unpack(target)
  for _, color in ipairs(palette) do
    local cr, cg, cb = table.unpack(color)
    local r = math.tointeger((cr - tr) * ratio + tr)
    local g = math.tointeger((cg - tg) * ratio + tg)
    local b = math.tointeger((cb - tb) * ratio + tb)
    table.insert(p, { r, g, b })
  end
  return p
end

function Main:__ctor()
  local palette = Palette.new("pico-8-ext")
  Display.palette(palette)

  Class.dump(System.args())

  local canvas = Canvas.default()
  canvas:transparent({ ["0"] = false, ["22"] = true })
  canvas:background(12)

  self.atlas = Canvas.new(1, 1)
  self.pixies = Bank.new(canvas, self.atlas, 1, 1)
  self.bank = Bank.new(canvas, Canvas.new("assets/sprites.png", 22), 16, 16)
  self.tileset = Bank.new(canvas, Canvas.new("assets/tileset.png", 22), 16, 16)
  self.batch = Batch.new(self.bank, 5000)
  self.font = Font.default(canvas, 22, 2)

  self.animations = {
      ["sleeping-right"] = Animation.new(self.bank, { 12 }, 0, nil, false, false),
      ["sleeping-left"] = Animation.new(self.bank, { 12 }, 0, nil, true, false),
      ["idle-right"] = Animation.new(self.bank, { 9, 10, 11 }, 0.25, "circular", false, false),
      ["idle-left"] = Animation.new(self.bank, { 9, 10, 11 }, 0.25, "circular", true, false),
      ["running-right"] = Animation.new(self.bank, { 17, 18, 19, 20, 21, 22 }, 0.1, "circular", false, false),
      ["running-left"] = Animation.new(self.bank, { 17, 18, 19, 20, 21, 22 }, 0.1, "circular", true, false),
      ["jumping-right"] = Animation.new(self.bank, { 25 }, 0, nil, false, false),
      ["jumping-left"] = Animation.new(self.bank, { 25 }, 0, nil, true, false),
      ["falling-right"] = Animation.new(self.bank, { 26 }, 0, nil, false, false),
      ["falling-left"] = Animation.new(self.bank, { 26 }, 0, nil, true, false)
    }
  self.facing = "right"
  self.animation = self.animations["idle-" .. self.facing]
  self.idle_time = nil
  self.map = generate_map(50)
  self.shake_time = 5

  self.position = Vector.new(25 * 15 * 16, 0)
  self.velocity = Vector.new(0, 0)
  self.acceleration = Vector.new(0, -9.81 * 0.75)
  self.jumps = 0

--[[
  self.snow = {}
  self.flake_time = 0
]]

  self.atlas:clear(0)

  -- Tweak the palette now that the loading phase is complete, so that color-remapping won't be interfered with!
  Display.palette(Palette.new(extra_half_brite(palette:colors(), { 31, 127, 63 }, 0.5)))
--  self.pixies:clear(0)
end

function Main:input()
  if self.jumps < 2 and Input.is_pressed("up") then
    self.velocity.y = 128
    self.jumps = self.jumps + 1
    self.idle_time = nil
  elseif Input.is_down("right") then
    self.facing = "right"
    self.velocity.x = 64
    self.idle_time = nil
  elseif Input.is_down("left") then
    self.facing = "left"
    self.velocity.x = -64
    self.idle_time = nil
  elseif Input.is_released("right") or Input.is_released("left") then
    self.velocity.x = 0
    self.idle_time = 0
  end

  if Input.is_pressed("start") then
    self.map = generate_map(50)
    self.shake_time = 5
  end
end

function Main:update(delta_time)
  self.velocity:add(self.acceleration)
  self.position:add(self.velocity:clone():scale(delta_time))

  if self.position.y <= 0 then
    self.position.y = 0
    self.velocity.y = 0
    self.jumps = 0
  end

  local animation
  if self.velocity.y > 0 then
    animation = self.animations["jumping-" .. self.facing]
  elseif self.velocity.y < 0 then
    animation = self.animations["falling-" .. self.facing]
  elseif self.velocity.x ~= 0 then
    animation = self.animations["running-" .. self.facing]
  elseif self.idle_time and self.idle_time >= 15 then
    animation = self.animations["sleeping-" .. self.facing]
  else
    animation = self.animations["idle-" .. self.facing]
  end
  if self.animation ~= animation then
    self.animation = animation
    self.animation:rewind()
  end

  self.animation:update(delta_time)

  if self.idle_time then
    self.idle_time = self.idle_time + delta_time
  end

  if self.shake_time > 0 then
    self.shake_time = self.shake_time * 0.5
    if self.shake_time < 0.01 then
      self.shake_time = 0
      Display.offset(0, 0)
    else
      local t = System.time()
      Display.offset(math.sin(t * 77 + 31) * 16, math.sin(t * 123 + 43) * 16)
    end
  end

  local canvas = Canvas.default()
  local width, height = canvas:size()
--[[
  self.flake_time = self.flake_time + delta_time
  while self.flake_time >= 0.025 do
    self.flake_time = self.flake_time - 0.025
    if #self.snow < 1024 then
      local v = 255
      local color = Display.color_to_index(v, v, v)
      table.insert(self.snow, {
          x = math.random(0, width - 1),
          y = -32,
          z = math.random(1, 5),
          angle = 0,
          vy = 24,
          vx = 0,
          va = math.random() * Math.SINCOS_PERIOD,
          color = color,
        })
    end
  end

  local wind_vx = 0 -- math.random(-128, 128)
  for index = #self.snow, 1, -1 do
    local flake = self.snow[index]
    local factor_x = -1.0 / flake.z
    local factor_y = 1.0 / flake.z
    flake.vx = self.velocity.x + wind_vx

    flake.x = flake.x + (flake.vx * delta_time) * factor_x
    flake.y = flake.y + (flake.vy * delta_time) * factor_y

    flake.angle = flake.angle + (flake.va * delta_time)

    if flake.x < 0 then
      flake.x = flake.x + width
    elseif flake.x >= width then
      flake.x = flake.x - width
    end

    if flake.y >= 96 then
      table.remove(self.snow, index)
    end
  end
]]

  local delta_y = self.position.y * 0.75
  local y = height * 0.5 + delta_y + 32

  local t = System.time()
  local program = Program.new()
  program:wait(0, y)
  for i = 0, 31 do
    program:shift(i, 32 + i)
  end
  program:modulo(-width * 2)
  for i = y, height - 1 do
    program:wait(0, i)
    program:offset(math.sin(t * 9.0 + i * 0.25) * WATER_DISPLACEMENT)
  end
  Display.program(program)
end

function Main:render(_)
  local canvas = Canvas.default()
  local width, height = canvas:size()
  canvas:clear()

  local x, y = (width - 16) * 0.5, height * 0.5

  self.animation:blit(x, y - self.position.y)

  local delta_y = self.position.y * 0.75

  y = y + delta_y

  local ox = math.tointeger(self.position.x / 16)
  local dx = self.position.x % 16
  for i = 1, 5 do
    for j = 1, 15 + 1 do
      local cell_id = self.map[i][ox + j]
      self.tileset:blit(cell_id, (j - 1) * 16 - dx, y + 16 + (i - 2) * 16)
    end
  end

--[[
  canvas:push()
  for _, flake in ipairs(self.snow) do
    canvas:shift(0, flake.color)
    local scale = 1--flake.z * 0.5
    self.pixies:blit(0, flake.x, flake.y + delta_y, scale, scale, flake.angle, 0.5, 0.5)
  end
  canvas:pop()
]]

--[[
  local t = System.time()
  local mid = math.tointeger(y) + 32
  local amount = height - mid
  for i = 0, amount - 1 do
      canvas:process(function(_, _, _, to)
--        local ar, ag, ab = Display.index_to_color(from)
        local ar, ag, ab = 31, 127, 63
        local br, bg, bb = Display.index_to_color(to)
        local r, g, b = (ar + br) * 0.5, (ag + bg) * 0.5, (ab + bb) * 0.5
        return Display.color_to_index(r, g, b)
      end, 0, mid + i, math.sin(t + i / (amount / 8)) * 3, mid - i * 1, width, 1)
  end
]]
  self.font:write(string.format("FPS: %d", math.floor(System.fps() + 0.5)), 0, 0)

--  local a, b, c, d = System.stats()
--  self.font:write(string.format("%.2f %.2f %.2f %.2f %.2f", a, b, c, d, 1 / d), 0, 8)
end

return Main