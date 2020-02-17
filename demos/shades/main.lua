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

local Input = require("tofu.events").Input
local Bank = require("tofu.graphics").Bank
local Canvas = require("tofu.graphics").Canvas
local Display = require("tofu.graphics").Display
local Font = require("tofu.graphics").Font
local Class = require("tofu.util").Class
local System = require("tofu.core").System

local PALETTE
local STEPS
local LEVELS
local TARGET = 0x00000000

local function get_r(c)
  return (c >> 16) & 0xff
end
local function get_g(c)
  return (c >> 8) & 0xff
end
local function get_b(c)
  return c & 0xff
end

local function find_best_match(palette, match)
  local index
  local min = math.huge
  for i, color in ipairs(palette) do
    local dr, dg, db = (get_r(match) - get_r(color)), (get_g(match) - get_g(color)), (get_b(match) - get_b(color))
    local d = dr * dr * 2.0 + dg * dg * 4.0 + db * db * 3.0
    if min > d then
      min = d
      index = i
    end
  end
  return index
end

local function build_table(palette, levels, target)
  local lut = {}
  for i = 0, levels - 1 do
    local shifting = {}
    local ratio = 1 / (levels - 1) * i
    for j, color in ipairs(palette) do
      local r = math.tointeger((get_r(color) - get_r(target)) * ratio + get_r(target))
      local g = math.tointeger((get_g(color) - get_g(target)) * ratio + get_g(target))
      local b = math.tointeger((get_b(color) - get_b(target)) * ratio + get_b(target))
      local k = find_best_match(palette, r * 65536 + g * 256 + b)
      shifting[j - 1] = k - 1
    end
    lut[i] = shifting
  end
  return lut
end

local Main = Class.define()

function Main:__ctor()
  Display.palette("pico-8")

  PALETTE = Display.palette()
  STEPS = #PALETTE
  LEVELS = STEPS

  local canvas = Canvas.default()
  local width, height = canvas:size()
  self.lut = build_table(PALETTE, LEVELS, TARGET)

  self.bank = Bank.new("assets/sheet.png", 8, 8)
  self.font = Font.default(0, 15)
  self.width = width / STEPS
  self.height = height / STEPS
  self.mode = 0
end

function Main:input()
  if Input.is_pressed("y") then
    self.mode = (self.mode + 1) % 10
  end
end

function Main:update(_)
end

function Main:render(_)
  local canvas = Canvas.default()
  local width, height = canvas:size()
  canvas:clear()

  for i = 0, STEPS - 1 do
    local y = self.height * i
    for j = 0, STEPS - 1 do
      local x = self.width * j
      canvas:rectangle("fill", x, y, self.width, self.height, (i + j) % STEPS)
    end
  end

  canvas:push()
  canvas:transparent({ [0] = false })
  if self.mode == 0 then
    for i = 0, STEPS - 1 do
      local y = self.height * i
      canvas:shift(self.lut[i])
      canvas:process(0, y, width, self.height)
    end
  elseif self.mode == 1 then
    for i = 0, STEPS - 1 do
      canvas:shift(self.lut[i])
      canvas:process(i, 0, 1, height)
      canvas:process(width - 1 - i, 0, 1, height)
    end
  else
    local t = System.time()
    local index = math.tointeger((math.sin(t * 2.5) + 1) * 0.5 * (STEPS - 1))
    canvas:shift(self.lut[index])
    canvas:process(0, 0, width, height / 2)
  end
  canvas:pop()

  self.font:write(string.format("FPS: %d", System.fps()), 0, 0)
end

return Main
