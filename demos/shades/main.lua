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
local Canvas = require("tofu.graphics").Canvas
local Display = require("tofu.graphics").Display
local Font = require("tofu.graphics").Font
local Palette = require("tofu.graphics").Palette

local PALETTE
local STEPS
local LEVELS
local TARGET = { 0, 0, 0 }

local function build_table(palette, levels, target)
  local tr, tg, tb = table.unpack(target)
  local lut = {}
  for i = 0, levels - 1 do
    local shifting = {}
    local ratio = 1 / (levels - 1) * i
    for j, color in ipairs(palette:colors()) do
      local ar, ag, ab = table.unpack(color)
      local r, g, b = Palette.mix(ar, ag, ab, tr, tg, tb, 1 - ratio)
      local k = palette:color_to_index(r, g, b)
      shifting[j - 1] = k - 1
    end
    lut[i] = shifting
  end
  return lut
end

local Main = Class.define()

function Main:__ctor()
  PALETTE = Palette.new("pico-8-ext")
  STEPS = PALETTE:size()
  LEVELS = STEPS

  Display.palette(PALETTE)

  local canvas = Canvas.default()
  local width, height = canvas:size()

  canvas:transparent(0, false)

  self.lut = build_table(PALETTE, LEVELS, TARGET)
  self.font = Font.default(canvas, 0, 15)
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
  canvas:transparent(0, false)
  if self.mode == 0 then
    for i = 0, STEPS - 1 do
      local y = self.height * i
      canvas:shift(self.lut[i])
      canvas:copy(0, y, 0, y, width, self.height)
    end
  elseif self.mode == 1 then
    for i = 0, STEPS - 1 do
      canvas:shift(self.lut[i])
      canvas:copy(i, 0, i, 0, 1, height)
      canvas:copy(width - 1 - i, 0, width - 1 - i, 0, 1, height)
    end
  else
    local t = System.time()
    local index = math.tointeger((math.sin(t * 2.5) + 1) * 0.5 * (STEPS - 1))
    canvas:shift(self.lut[index])
    canvas:copy(0, 0, 0, 0, width, height / 2)
  end
  canvas:pop()

  self.font:write(string.format("FPS: %d", System.fps()), 0, 0)
end

return Main
