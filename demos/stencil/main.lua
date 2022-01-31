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

local Class = require("tofu.core.class")
local System = require("tofu.core.system")
local Input = require("tofu.events.input")
local Canvas = require("tofu.graphics.canvas")
local Display = require("tofu.graphics.display")
local Font = require("tofu.graphics.font")
local Palette = require("tofu.graphics.palette")

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
  local color = Palette.default("famicube")
  local greyscale = Palette.new(color:size())

  self.font = Font.default(0, 15)
  self.top = Canvas.new("assets/top.png", 0, color)
  self.bottom = Canvas.new("assets/bottom.png", 0, color)
  self.mask = Canvas.new("assets/gradient.png", 0, greyscale)
  self.comparator = 1
  self.threshold = 255
  self.mode = 0

  --  canvas:transparent(0, false)

  Display.palette(color)
end

function Main:process()
  if Input.is_pressed("select") then
    self.mode = (self.mode + 1) % 2
  elseif Input.is_pressed("up") then
    self.comparator = math.min(self.comparator + 1, #COMPARATORS)
  elseif Input.is_pressed("down") then
    self.comparator = math.max(self.comparator - 1, 1)
  elseif Input.is_pressed("right") then
    self.threshold = self.mode == 1 and self.threshold or math.min(self.threshold + 1, 255)
  elseif Input.is_pressed("left") then
    self.threshold = self.mode == 1 and self.threshold or math.max(self.threshold - 1, 0)
  end
end

function Main:update(_)
  if self.mode == 1 then
    local t = System.time()
    self.threshold = math.tointeger(((math.sin(t) + 1) * 0.5) * 255 + 0.5)
  end
end

function Main:render(_)
  local canvas = Canvas.default()
  canvas:clear(0)

  canvas:copy(self.bottom)
  -- self.top:process(function(x, y, from, to)
  --     local pixel = self.mask:peek(x, y)
  --     if pixel < self.threshold then
  --       return from
  --     else
  --       return to
  --     end
  --   end, canvas)
  canvas:stencil(self.top, self.mask, COMPARATORS[self.comparator], self.threshold)

  local width, _ = canvas:size()
  self.font:write(canvas, 0, 0, string.format("FPS: %.1f", System.fps()))
  self.font:write(canvas, width, 0, string.format("M: %d, T: %d", self.mode, self.threshold), "right")
end

return Main
