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

local COMPARATORS <const> = {
  "never",
  "less",
  "less-or-equal",
  "greater",
  "greater-or-equal",
  "equal",
  "not-equal",
  "always",
}

local Main = Class.define()

function Main:__ctor()
  local color = Palette.new("famicube")
  local greyscale = Palette.new(color:size())

  local canvas = Canvas.default()

  self.font = Font.default(canvas, 0, 15)
  self.top = Canvas.new("assets/top.png", 0, color)
  self.bottom = Canvas.new("assets/bottom.png", 0, color)
  self.stencil = Canvas.new("assets/gradient.png", 0, greyscale)
  self.comparator = 1
  self.threshold = 64
  self.mode = 0

  --  canvas:transparent(0, false)
  canvas:comparator(COMPARATORS[self.comparator])
  canvas:threshold(self.threshold)

  Display.palette(color)
end

function Main:input()
  if Input.is_pressed("select") then
    self.mode = (self.mode + 1) % 2
  elseif Input.is_pressed("up") then
    self.comparator = math.min(self.comparator + 1, #COMPARATORS)
    local canvas = Canvas.default()
    canvas:comparator(COMPARATORS[self.comparator])
  elseif Input.is_pressed("down") then
    self.comparator = math.max(self.comparator - 1, 1)
    local canvas = Canvas.default()
    canvas:comparator(COMPARATORS[self.comparator])
  elseif Input.is_pressed("right") then
    self.threshold = self.mode == 1 and self.threshold or math.min(self.threshold + 1, 64)
    local canvas = Canvas.default()
    canvas:threshold(self.threshold)
  elseif Input.is_pressed("left") then
    self.threshold = self.mode == 1 and self.threshold or math.max(self.threshold - 1, 0)
    local canvas = Canvas.default()
    canvas:threshold(self.threshold)
  end
end

function Main:update(_)
  if self.mode == 1 then
    local t = System.time()
    self.threshold = math.tointeger(((math.sin(t) + 1) * 0.5) * 64 + 0.5)
    local canvas = Canvas.default()
    canvas:threshold(self.threshold)
  end
end

function Main:render(_)
  local canvas = Canvas.default()
  canvas:clear()

  self.bottom:copy(canvas)
  -- self.top:process(function(x, y, from, to)
  --     local pixel = self.stencil:peek(x, y)
  --     if pixel < self.threshold then
  --       return from
  --     else
  --       return to
  --     end
  --   end, canvas)
  self.top:mask(self.stencil, canvas)

  local width, _ = canvas:size()
  self.font:write(string.format("FPS: %.1f", System.fps()), 0, 0)
  self.font:write(self.font:align(string.format("M: %d, T: %d", self.mode, self.threshold), width, 0, "right"))
end

return Main
