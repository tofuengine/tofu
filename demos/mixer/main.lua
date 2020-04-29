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
--local Input = require("tofu.events").Input
local Canvas = require("tofu.graphics").Canvas
local Display = require("tofu.graphics").Display
local Font = require("tofu.graphics").Font
local Group = require("tofu.sound").Group
local Source = require("tofu.sound").Source

local Main = Class.define()

function Main:__ctor()
  Display.palette("pico-8")

  self.font = Font.default(0, 15)

  local group = Group.default()
  self.sources = {
--      Source.new("assets/44100_mono.wav", group),
      Source.new("assets/48000_2ch.wav", group),
    }
end

function Main:input()
--[[
  for _, id in ipairs(IDS) do
    self.down[id] = Input.is_down(id)
    if Input.is_pressed(id) then
      self.scale[id] = 3.0
    end
  end
]]--
end

function Main:update(_)
end

function Main:render(_)
  local canvas = Canvas.default()
  canvas:clear()

  local width, height = canvas:size()

  local x, y = 0, 0
  for index, source in ipairs(self.sources) do
    local text = string.format("[%d] %.3f .3f", index, source:pan(), source:gain())
    local _, th = self.font:size(text)
    self.font:write(text, x, y)
    y = y + th
  end

  self.font:write(self.font:align(string.format("FPS: %d", System.fps()), width, height, "right", "bottom"))
end

return Main
