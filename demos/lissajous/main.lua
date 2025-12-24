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
local Palette = require("tofu.graphics.palette")
--local Bank = require("tofu.graphics.bank")
local Font = require("tofu.graphics.font")
local Image = require("tofu.graphics.image")
local Controller = require("tofu.input.controller")

local PALETTE <const> = Palette.new(256)
local FONT <const> = Font.default()
local CANVAS <const> = Canvas.default()
local WIDTH <const>, HEIGHT <const> = CANVAS:image():size()
local HALF_WIDTH <const>, HALF_HEIGHT <const> = WIDTH * 0.5, HEIGHT * 0.5

-- See: https://en.wikipedia.org/wiki/Lissajous_curve

local DRAWS_PER_FRAME = 25
local STEP = 0.0005

local Main = Class.define()

local function _create_bank()
  local image = Image.new(16, 16)
  image:clear(1)
  return image
--  return Bank.new(image, 2, 2)
end

function Main:__ctor()
  self.bank = _create_bank();

  self.a = 1
  self.b = 1
  self.delta = math.pi / 4
  self.A = HALF_WIDTH - 8
  self.B = HALF_HEIGHT - 8

  self.age = 0
end

function Main:init()
  Display.palette(PALETTE)
end

function Main:deinit()
end

function Main:handle_input(_)
  local controller = Controller.default()

  if controller:is_pressed("start") then
    local canvas = Canvas.default()
    canvas:image():clear(0)
  end

  if controller:is_down("right") then
    self.a = self.a + 1
  elseif controller:is_down("left") then
    self.a = self.a - 1
  end

  if controller:is_down("down") then
    self.b = self.b + 1
  elseif controller:is_down("up") then
    self.b = self.b - 1
  end
end

function Main:update(_)
  self:handle_input(_)
end

function Main:render(canvas, _)
  --!!! DON'T CLEAR THE CANVAS !!!

  local t = self.age
  for _ = 1, DRAWS_PER_FRAME do
    t = t + STEP
    local x = HALF_WIDTH + math.cos(t * self.a + self.delta) * self.A
    local y = HALF_HEIGHT + math.sin(t * self.b) * self.B

    canvas:blend(x - 8, y - 8, self.bank, 'add')
  end
  self.age = t

  canvas:write(0, 0, FONT, string.format("%d FPS", System.fps()))
end

return Main