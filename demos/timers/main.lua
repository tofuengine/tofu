--[[
                ___________________  _______________ ___
                \__    ___/\_____  \ \_   _____/    |   \
                  |    |    /   |   \ |    __) |    |   /
                  |    |   /    |    \|     \  |    |  /
                  |____|   \_______  /\___  /  |______/
                                   \/     \/
        ___________ _______    ________.___ _______  ___________
        \_   _____/ \      \  /  _____/|   |\      \ \_   _____/
         |    __)_  /   |   \/   \  ___|   |/   |   \ |    __)_
         |        \/    |    \    \_\  \   /    |    \|        \
        /_______  /\____|__  /\______  /___\____|__  /_______  /
                \/         \/        \/            \/        \

MIT License

Copyright (c) 2019-2024 Marco Lizza

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
local Pool = require("tofu.timers.pool")

local Main = Class.define()

function Main:__ctor()
  Display.palette(Palette.default("pico-8"))

  local canvas = Canvas.default()
  local image = canvas:image()
  local width, height = image:size()

  self.pool = Pool.new()

  self.timerA = self.pool:spawn(0.5, 50, function()
      --local o = Object.new()
      self.x = math.random() * width
    end)
  self.timerB = self.pool:spawn(0.25, -1, function()
      self.y = math.random() * height
    end)
  self.timerC = self.pool:spawn(15, 0, function()
      self.timerA:cancel()
      self.timerB = nil
    end)

    self.x = math.random() * width
    self.y = math.random() * height
end

function Main:init()
end

function Main:update(delta_time)
  self.pool:update(delta_time)
end

function Main:render(_)
  --local x = X.new()
  local canvas = Canvas.default()
  local image = canvas:image()
  image:clear(0)

  canvas:circle("fill", self.x, self.y, 5, 15)
end

return Main
