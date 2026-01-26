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

Copyright (c) 2019-2026 Marco Lizza

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
local Controller = require("tofu.input.controller")
local Bank = require("tofu.graphics.bank")
local Canvas = require("tofu.graphics.canvas")
local Display = require("tofu.graphics.display")
local Font = require("tofu.graphics.font")
local Palette = require("tofu.graphics.palette")

local PALETTE <const> = Palette.default("nes")
local FONT <const> = Font.default()
local CANVAS <const> = Canvas.default()
local WIDTH <const>, HEIGHT <const> = CANVAS:image():size()

local CX <const> = WIDTH * 0.5
local CY <const> = HEIGHT * 0.5

local Main = Class.define()

function Main:__ctor()
  self.bank = Bank.from_image("assets/sprites.img", 16, 16)

  self.anchor = 0.5
  self.anchor_speed = 0.0
  self.rotation = 128
  self.rotation_speed = 0.0
  self.scale = 2.0
  self.scale_speed = 0.0
  self.flip_x = false
  self.flip_y = false
end

function Main:init()
  Display.palette(PALETTE)
end

function Main:deinit()
end

function Main:handle_input()
  local controller = Controller.default()

  self.scale_speed = 0
  if controller:is_down("up") then
    self.scale_speed = self.scale_speed + 2
  end
  if controller:is_down("down") then
    self.scale_speed = self.scale_speed - 2
  end

  self.rotation_speed = 0
  if controller:is_down("left") then
    self.rotation_speed = self.rotation_speed - 32
  end
  if controller:is_down("right") then
    self.rotation_speed = self.rotation_speed + 32
  end

  self.anchor_speed = 0
  if controller:is_down("a") then
    self.anchor_speed = self.anchor_speed - 1
  end
  if controller:is_down("b") then
    self.anchor_speed = self.anchor_speed + 1
  end

  if controller:is_pressed("x") then
    self.flip_x = not self.flip_x
  end
  if controller:is_pressed("y") then
    self.flip_y = not self.flip_y
  end
end

function Main:update(delta_time)
  self:handle_input()

  self.scale = math.max(0, self.scale + self.scale_speed * delta_time)
  self.rotation = self.rotation + self.rotation_speed * delta_time
  self.anchor = math.min(1.0, math.max(0.0, self.anchor + self.anchor_speed * delta_time))
end

function Main:render(canvas, _)
  canvas:clear(45)

  for _ = 1, 1 do
    canvas:sprite(CX, CY, self.bank, 9,
      self.flip_x and -self.scale or self.scale, self.flip_y and -self.scale or self.scale,
      self.rotation,
      self.anchor, self.anchor)
  end

  canvas:write(0, 0, FONT, string.format("%d FPS", math.floor(System.fps() + 0.5)))
  canvas:write(WIDTH, HEIGHT, FONT, string.format("S:%.2f|R:%4d|A:%.2f", self.scale, self.rotation, self.anchor),
    "right", "bottom")
end

return Main