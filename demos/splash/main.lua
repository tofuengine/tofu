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
local Canvas = require("tofu.graphics").Canvas
local Display = require("tofu.graphics").Display
local Palette = require("tofu.graphics").Palette
local Source = require("tofu.sound").Source

local Background = require("libs.background")
local Logo = require("libs.logo")
local Stars = require("libs.stars")
local Wave = require("libs.wave")

local Main = Class.define()

function Main:__ctor()
  local palette = Palette.new("famicube") -- Pick a 64 colors palette.
  Display.palette(palette)

  local canvas = Canvas.default()
  canvas:background(63) -- Use index #63 as background colour and transparent! :)
  canvas:transparent({ [0] = false, [63] = true })

  self.objects = {
      Background.new(canvas, 63, palette),
      Wave.new(canvas, 63),
      Logo.new(canvas, 63),
      Stars.new(canvas, 63)
    }

  self.music = Source.new("assets/modules/a_nice_and_warm_day.mod", "module")
  self.music:looped(true)
  self.music:play()
end

function Main:input()
end

function Main:update(delta_time)
  for _, object in ipairs(self.objects) do
    object:update(delta_time)
  end
end

function Main:render(_)
  local canvas = Canvas.default()
  canvas:clear()

  for _, object in ipairs(self.objects) do
    object:render()
  end
end

return Main
