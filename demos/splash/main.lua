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
local Canvas = require("tofu.graphics.canvas")
local Display = require("tofu.graphics.display")
local Palette = require("tofu.graphics.palette")
local Source = require("tofu.sound.source")

local Background = require("lib/background")
local Logo = require("lib/logo")
local Stars = require("lib/stars")
local Wave = require("lib/wave")

local Main = Class.define()

function Main:__ctor()
  local palette = Palette.default("famicube")
  local background_index = palette:size() - 1 -- The background index is the last palette entry!
  Display.palette(palette)

  local canvas = Canvas.default()
  canvas:background(background_index) -- Use it as background colour and transparent! :)
  canvas:transparent({ [0] = false, [background_index] = true })

  self.objects = {
      Background.new(canvas, background_index, palette),
      Wave.new(canvas, background_index, palette),
      Logo.new(canvas, background_index, palette),
      Stars.new(canvas, background_index, palette)
    }

  self.music = Source.new("assets/modules/a_nice_and_warm_day.mod", "module")
  self.music:looped(true)
  self.music:play()
end

function Main:process()
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
    object:render(canvas)
  end
end

return Main
