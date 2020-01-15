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

local Canvas = require("tofu.graphics").Canvas
local Font = require("tofu.graphics").Font
local Class = require("tofu.util").Class
local System = require("tofu.core").System

local Main = Class.define()

local MESSAGE = "Hello, Tofu!"

function Main:__ctor()
  Canvas.palette("pico-8")

  self.font = Font.default(0, 15)
end

function Main:input()
end

function Main:update(_)
end

function Main:render(_)
  local t = System.time()

  Canvas.clear()

  local font_width, font_height = self.font:width(), self.font:height()

  local x, y = (Canvas.width() - #MESSAGE * font_width) * 0.5, (Canvas.height() - font_height) * 0.5

  for c in MESSAGE:gmatch(".") do
    local dy = math.sin(t * 2.5 + x * 0.75) * font_height * 1.5

    self.font:write(c, x, y + dy)

    x = x + font_width
  end

  self.font:write(string.format("FPS: %d", System.fps()), 0, 0)
end

return Main
