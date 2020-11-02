--[[
MIT License

Copyright (c) 2019-2020 Marco Lizza

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
  canvas:transparent({ ["0"] = false, ["22"] = false })
  canvas:background(12)

  self.bank = Bank.new(canvas, Canvas.new("assets/sprites.png"), 16, 16)
  self.tileset = Bank.new(canvas, Canvas.new("assets/tileset.png"), 16, 16)
  self.batch = Batch.new(self.bank, 5000)
  self.font = Font.default(canvas, 22, 6)

  self.animations = {
      ["sleeping-right"] = Animation.new(self.bank, { 12 }, 0.1, "circular", false, false),
      ["sleeping-left"] = Animation.new(self.bank, { 12 }, 0.1, "circular", true, false),
      ["idle-right"] = Animation.new(self.bank, { 9, 10, 11 }, 0.25, "circular", false, false),
      ["idle-left"] = Animation.new(self.bank, { 9, 10, 11 }, 0.25, "circular", true, false),
      ["running-right"] = Animation.new(self.bank, { 17, 18, 19, 20, 21, 22 }, 0.1, "circular", false, false),
      ["running-left"] = Animation.new(self.bank, { 17, 18, 19, 20, 21, 22 }, 0.1, "circular", true, false)
    }
  self.facing = "right"
  self.animation = self.animations["idle-" .. self.facing]
  self.idle_time = nil
  self.map = generate_map(50)
  self.offset = 25 * 15 * 16
  self.velocity = 0
end

function Main:input()
  local animation = self.animation

  if Input.is_down("right") then
    self.facing = "right"
    self.velocity = 1
    self.idle_time = nil
    animation = self.animations["running-" .. self.facing]
  elseif Input.is_down("left") then
    self.facing = "left"
    self.velocity = -1
    self.idle_time = nil
    animation = self.animations["running-" .. self.facing]
  elseif Input.is_released("right") or Input.is_released("left") then
    self.velocity = 0
    self.idle_time = 0
    animation = self.animations["idle-" .. self.facing]
  end

  if Input.is_pressed("start") then
    self.map = generate_map(50)
  end

  if self.idle_time and self.idle_time >= 15 then
    animation = self.animations["sleeping-" .. self.facing]
  end

  if self.animation ~= animation then
    self.animation = animation
    self.animation:rewind()
  end
end

function Main:update(delta_time)
  self.animation:update(delta_time)

  self.offset = self.offset + self.velocity * 96 * delta_time

  if self.idle_time then
    self.idle_time = self.idle_time + delta_time
  end
end

function Main:render(_)
  local canvas = Canvas.default()
  local width, height = canvas:size()
  canvas:clear()

  canvas:square("fill", 16, 16, 16 * 2, 20)
  canvas:square("fill", 32, 16, 16 * 2, 21)
  self.bank:blit(12, 32, 16, -2.0, 2.0, 0.0, 0.0, 0.0)

  canvas:square("fill", 16, 48, 16 * 2, 20)
  canvas:square("fill", 32, 48, 16 * 2, 21)
  self.bank:blit(12, 32, 48,  2.0, 2.0, 0.0, 0.0, 0.0)

  canvas:square("fill", 16, 80, 16 * 2, 20)
  canvas:square("fill", 32, 80, 16 * 2, 21)
  self.bank:blit(12, 32, 80, 2.0, 2.0, 0.0, 0.0, 0.0)

--  self.bank:blit(12, 64, 16, -1.0, 1.0, 0.0, 1.0, 1.0)
--  self.bank:blit(12, 64, 32,  1.0, 1.0, 0.0, 1.0, 1.0)
--  self.bank:blit(12, 64, 48, 1.0, 1.0, 0.0, 1.0, 1.0)

--  self.bank:blit(12, 96, 16, -1.0, 1.0, 0.0, 0.25, 0.25)
--  self.bank:blit(12, 96, 32,  1.0, 1.0, 0.0, 0.25, 0.25)
--  self.bank:blit(12, 96, 48, 1.0, 1.0, 0.0, 0.25, 0.25)

  local x, y = (width - 16) * 0.5, height * 0.5

  self.animation:blit(x, y)
--[[
  local ox = math.tointeger(self.offset / 16)
  local dx = self.offset % 16
  for i = 1, 5 do
    for j = 1, 15 + 1 do
      local cell_id = self.map[i][ox + j]
      self.tileset:blit(cell_id, (j - 1) * 16 - dx, y + 16 + (i - 2) * 16)
    end
  end
]]
  self.font:write(string.format("FPS: %d", System.fps()), 0, 0)
end

return Main