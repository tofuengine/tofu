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

local PALETTE <const> = Palette.default("nes")
local PALETTE_FONT <const> = Palette.new({{ 0, 255, 0 }})
local FONT <const> = Font.default()
local CANVAS <const> = Canvas.default()
local WIDTH <const>, HEIGHT <const> = CANVAS:image():size()

local SPRITE_WIDTH <const> = 16
local SPRITE_HEIGHT <const> = 16

local SCALE <const> = 2.0

local STEP_X <const> = SPRITE_WIDTH * SCALE
local STEP_Y <const> = SPRITE_HEIGHT * SCALE

local Main = Class.define()

function Main:__ctor()
  self.bank = Bank.from_image("assets/sprites.img", SPRITE_WIDTH, SPRITE_HEIGHT)

  local rotation = 0

  self.sprites = {}
  for i = 0, WIDTH, STEP_X do
    for j = 0, HEIGHT, STEP_Y do
      table.insert(self.sprites, {
          x = i, y = j,
          pivot = 0.5,
          rotation = rotation,
          scale = SCALE,
          flip_x = false,
          flip_y = false
        })
      rotation = rotation + 64
    end
  end
end

function Main:init()
  Display.palette(PALETTE, 0)
  Display.palette(PALETTE_FONT, 1)
end

function Main:deinit()
end

function Main:update(_)
end

function Main:render(canvas, _)
  canvas:clear(45)

  for _, sprite in ipairs(self.sprites) do
    canvas:sprite(sprite.x, sprite.y,
      self.bank, 9,
      sprite.flip_x and -sprite.scale or sprite.scale,
      sprite.flip_y and -sprite.scale or sprite.scale,
      sprite.rotation,
      sprite.pivot, sprite.pivot)
  end

  canvas:push()
    canvas:bank(1)
    canvas:write(0, 0, FONT, string.format("%d FPS", math.floor(System.fps() + 0.5)))
  canvas:pop()
end

return Main