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
local Class = require("tofu.util").Class
local Timer = require("tofu.util").Timer

local Main = Class.define()

function Main:__ctor()
  Canvas.palette("pico-8")

  self.timerA = Timer.new(0.5, 50, function()
      --local o = Object.new()
      self.x = math.random() * Canvas.width()
    end)
  self.timerB = Timer.new(0.25, -1, function()
      self.y = math.random() * Canvas.height()
    end)
  self.timerC = Timer.new(15, 0, function()
      self.timerA:cancel()
      self.timerB = nil
    end)

    self.x = math.random() * Canvas.width()
    self.y = math.random() * Canvas.height()
end

function Main:input()
end

function Main:update(_)
end

function Main:render(_)
  --local x = X.new()
  Canvas.clear()
  Canvas.circle("fill", self.x, self.y, 5, 15)
end

return Main
