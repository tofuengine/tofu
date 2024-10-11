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
local System = require("tofu.core.system")
local Canvas = require("tofu.graphics.canvas")
local Display = require("tofu.graphics.display")
local Image = require("tofu.graphics.image")
local Font = require("tofu.graphics.font")
local Palette = require("tofu.graphics.palette")

local Main = Class.define()

local MARGIN <const> = 16
local POINTS_PER_SECOND <const> = 8192
local RESET_AFTER <const> = 10

local function _mid_point(a, b)
  return {
    x = (a.x + b.x) * 0.5,
    y = (a.y + b.y) * 0.5
  }
end

function Main:__ctor()
  Display.palette(Palette.default("pico-8"))

  self.font = Font.default(0, 1)

  local canvas = Canvas.default()
  local image = canvas:image()
  local width, height = image:size()

  self.buffer = Canvas.new(Image.new(width, height))

  self.age = math.huge
end

function Main:init()
end

function Main:deinit()
end

function Main:_generate_point()
  local index = math.random(1, 3)
  local point = self.points[index]
  local mid_point = _mid_point(self.last_point, point)
  self.last_point = mid_point
  return index
end

function Main:_reset()
  local canvas = self.buffer
  local image = canvas:image()
  local width, height = image:size()

  image:clear(0)

  self.points = {}
  for index = 1, 3 do
    local x = math.random(MARGIN, width - MARGIN - 1)
    local y = math.random(MARGIN, height - MARGIN - 1)
    table.insert(self.points, { x = x, y = y })

    canvas:point(x, y, index)
  end

  local a = self.points[1]
  local b = self.points[2]
  self.last_point = _mid_point(a, b)
  canvas:point(self.last_point.x, self.last_point.y, 2)
end

function Main:update(delta_time)
  self.age = self.age + delta_time
  if self.age >= RESET_AFTER then
    self.age = 0
    self:_reset()
  end

  local points = math.floor(POINTS_PER_SECOND * delta_time)
  local canvas = self.buffer
  for _ = 1, points do
    local index = self:_generate_point()
    canvas:point(self.last_point.x, self.last_point.y, index)
  end
end

function Main:render(_) -- ratio
  local canvas = Canvas.default()

  canvas:copy(self.buffer:image())

  canvas:write(0, 0, self.font, string.format("%d FPS", System.fps()))
end

return Main
