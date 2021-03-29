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

-- Include the modules we'll be using.
local Class = require("tofu.core").Class
local Input = require("tofu.events").Input
local Canvas = require("tofu.graphics").Canvas
local Display = require("tofu.graphics").Display
local Bank = require("tofu.graphics").Bank
local Font = require("tofu.graphics").Font
local Palette = require("tofu.graphics").Palette

local Main = Class.define()

function Main:__ctor()
  Display.palette(Palette.new("pico-8"))

  self.canvas = Canvas.new(Canvas.default():size())
  self.bank = Bank.new(self.canvas, Canvas.new("assets/font-8x8.png", 0), 8, 8)
  self.font = Font.new(self.canvas, Canvas.new("assets/font-8x8.png", 0, 15), 8, 8)
end

function Main:input()
  if Input.is_pressed("select") then
    if self.canvas then
      local canvas = Canvas.default()
      self.canvas = nil -- It shouldn't be GC-ed as long as bank/font reference it.
      self.bank:canvas(canvas)
      self.font:canvas(canvas)
    elseif self.bank then
      self.bank = nil
    elseif self.font then
      self.font = nil
    end
  end
end

function Main:update(_)
end

function Main:render(_)
  local canvas = Canvas.default()
  canvas:clear()
  local x, y = canvas:center()
  if self.font then
    self.font:write(self.font:align("Hello, Tofu!", x, y, "center", "middle"))
  end
end

return Main
