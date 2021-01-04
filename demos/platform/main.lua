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
local Vector = require("tofu.util").Vector

local Animation = require("lib.animation")

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

function Main:__ctor()
  Display.palette("pico-8-ext")

  local canvas = Canvas.default()
  canvas:transparent({ ["0"] = false, ["22"] = true })
  canvas:background(12)

  self.bank = Bank.new(canvas, Canvas.new("assets/sprites.png"), 16, 16)
  self.tileset = Bank.new(canvas, Canvas.new("assets/tileset.png"), 16, 16)
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

  self.snow = {}
  self.flake_time = 0
end

function Main:input()
  if self.jumps < 2 and Input.is_pressed("up") then
    self.velocity.y = 128
    self.jumps = self.jumps + 1
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
  local width, _ = canvas:size()

  self.flake_time = self.flake_time + delta_time
  while self.flake_time >= 0.025 do
    self.flake_time = self.flake_time - 0.025
    if #self.snow < 1024 then
      local v = 255
      local color = Display.color_to_index(v, v, v)
      table.insert(self.snow, {
          x = math.random(0, width - 1),
          y = 0,
          z = math.random(1, 5),
          vy = 12,
          vx = 0,
          color = color
        })
    end
  end

  local zombies = {}
  for index, flake in ipairs(self.snow) do
    local factor = -1.0 / flake.z
    flake.vx = self.velocity.x * factor

    flake.x = flake.x + flake.vx * delta_time
    flake.y = flake.y + flake.vy * delta_time

    if flake.x < 0 then
      flake.x = flake.x + width
    elseif flake.x >= width then
      flake.x = flake.x - width
    end

    if flake.y >= 96 then
      table.insert(zombies, index)
    end
  end

  for _, index in ipairs(zombies) do
      table.remove(self.snow, index)
  end
end

function Main:render(_)
  local canvas = Canvas.default()
  local width, height = canvas:size()
  canvas:clear()

  local x, y = (width - 16) * 0.5, height * 0.5

  self.animation:blit(x, y - self.position.y)

  y = y + self.position.y * 0.75

  local ox = math.tointeger(self.position.x / 16)
  local dx = self.position.x % 16
  for i = 1, 5 do
    for j = 1, 15 + 1 do
      local cell_id = self.map[i][ox + j]
      self.tileset:blit(cell_id, (j - 1) * 16 - dx, y + 16 + (i - 2) * 16)
    end
  end

  for _, flake in ipairs(self.snow) do
    canvas:point(flake.x, flake.y, flake.color)
  end

--[[
  local mid = math.tointeger(y) + 32
  local amount = height - mid
  for i = 0, amount - 1 do
      canvas:process(function(from, to)
        local ar, ag, ab = Display.index_to_color(from)
        local br, bg, bb = Display.index_to_color(to)
        local r, g, b = (ar + br) * 0.5, (ag + bg) * 0.5, (ab + bb) * 0.5
        return Display.color_to_index(r, g, b)
      end, 0, mid + i, 0, mid - i * 1, width, 1)
  end
--]]
  self.font:write(string.format("FPS: %d", math.floor(System.fps() + 0.5)), 0, 0)
end

return Main