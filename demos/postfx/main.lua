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
local Bank = require("tofu.graphics.bank")
local Canvas = require("tofu.graphics.canvas")
local Display = require("tofu.graphics.display")
local Font = require("tofu.graphics.font")
local Palette = require("tofu.graphics.palette")
local Controller = require("tofu.input.controller")

local Sprite = require("lib/sprite")

local PALETTE <const> = Palette.default("nes")
local PALETTE_FONT <const> = Palette.new({{ 0, 255, 0 }})
local FONT <const> = Font.default()
local CANVAS <const> = Canvas.default()
local WIDTH <const>, HEIGHT <const> = CANVAS:image():size()
local CONTROLLER <const> = Controller.default()

local LITTER_SIZE = 64

local Main = Class.define()

function Main:__ctor()
  self.sprites = {}
  self.bank = Bank.from_image("assets/images/diamonds.img", 16, 16)
  self.speed = 1.0
  self.running = true
end

function Main:init()
  Display.palette(PALETTE, 0)
  Display.palette(PALETTE_FONT, 1)
end

function Main:deinit()
end

function Main:handle_input()
  if CONTROLLER:is_pressed("start") then
    for _ = 1, LITTER_SIZE do
      table.insert(self.sprites, Sprite.new(self.bank, #self.sprites, CANVAS, WIDTH, HEIGHT))
    end
  elseif CONTROLLER:is_pressed("left") then
    self.speed = self.speed * 0.5
  elseif CONTROLLER:is_pressed("right") then
    self.speed = self.speed * 2.0
  elseif CONTROLLER:is_pressed("down") then
    self.speed = 1.0
  elseif CONTROLLER:is_pressed("select") then
    self.sprites = {}
  elseif CONTROLLER:is_pressed("y") then
    self.running = not self.running
  end
end

function Main:update(delta_time)
  self:handle_input()

  if not self.running then
    return
  end
  for _, sprite in pairs(self.sprites) do
    sprite:update(delta_time * self.speed)
  end
end

function Main:render(_)
  local canvas <const> = CANVAS
  local state <const> = CANVAS:state() -- TODO: pre-declare in the header

  canvas:clear(0)

  for _, sprite in pairs(self.sprites) do
    sprite:render(canvas)
  end

  state:push()
    state:bank(1)
    canvas:write(0, 0, FONT, string.format("%d FPS", System.fps()))
    canvas:write(WIDTH, 0, FONT, string.format("#%d sprites", #self.sprites), "right")
  state:pop()
end

return Main
