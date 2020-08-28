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
local Canvas = require("tofu.graphics").Canvas
local Display = require("tofu.graphics").Display
local Font = require("tofu.graphics").Font
local Source = require("tofu.sound").Source

local Main = Class.define()

function Main:__ctor()
  Display.palette("pico-8")

--  warn("@on")
--  warn("Hello,", "Again")

  self.font = Font.default(0, 15)

  self.sources = {
      Source.new("assets/1ch-22050-16.flac", Source.SAMPLE),
      Source.new("assets/2ch-48000-16.flac", Source.MUSIC),
      Source.new("assets/turrican_iii.xm", Source.MODULE),
--      Source.new("assets/tests/tone-portamento.xm", Source.MODULE),
    }
  --self.sources[1]:looping(false)
  self.sources[3]:play()


  self.current = 1
end

function Main:input()
  local source = self.sources[self.current]
  if Input.is_pressed("a") then
    source:play()
  elseif Input.is_pressed("b") then
    source:stop()
  elseif Input.is_pressed("x") then
    self.current = (self.current % #self.sources) + 1
  elseif Input.is_pressed("up") then
    local gain = source:gain()
    source:gain(gain + 0.05)
  elseif Input.is_pressed("down") then
    local gain = source:gain()
    source:gain(gain - 0.05)
  elseif Input.is_pressed("left") then
    local pan = source:pan()
    source:pan(pan - 0.05)
  elseif Input.is_pressed("right") then
    local pan = source:pan()
    source:pan(pan + 0.05)
  end
end

function Main:update(_)
end

function Main:render(_)
  local canvas = Canvas.default()
  canvas:clear()

  local width, height = canvas:size()

  local x, y = 0, 0
  for index, source in ipairs(self.sources) do
    local text = string.format("%s%s %.3f %.3f",
        self.current == index and "*" or " ",
        source:is_playing() and "!" or " ",
        source:pan(), source:gain())
    local _, th = self.font:size(text)
    self.font:write(text, x, y)
    y = y + th
  end

  self.font:write(self.font:align(string.format("FPS: %d", System.fps()), width, height, "right", "bottom"))
end

return Main
