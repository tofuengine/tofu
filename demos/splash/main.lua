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

local Class <const> = require("tofu.core.class")
local Canvas <const> = require("tofu.graphics.canvas")
local Display <const> = require("tofu.graphics.display")
local Palette <const> = require("tofu.graphics.palette")
local Source <const> = require("tofu.sound.source")
local Pool <const> = require("tofu.timers.pool")

local Background <const> = require("lib/background")
local Logo <const> = require("lib/logo")
local Stars <const> = require("lib/stars")
local Wave <const> = require("lib/wave")

local PALETTE <const> = Palette.default("nes")
local PALETTE_FONT <const> = Palette.new({{ 0, 255, 0 }})
local CANVAS <const> = Canvas.default()
local WIDTH <const>, HEIGHT <const> = CANVAS:image():size()
local STATE <const> = CANVAS:state()

local Main <const> = Class.define()

function Main:__ctor()
  self.pool = Pool.new()

  self.objects = {
      Background.new(WIDTH, HEIGHT, PALETTE, self.pool),
      Wave.new(WIDTH, HEIGHT, PALETTE, self.pool),
      Logo.new(WIDTH, HEIGHT, PALETTE, self.pool),
      Stars.new(WIDTH, HEIGHT, PALETTE, self.pool)
    }

  self.music = Source.new("assets/modules/a_nice_and_warm_day.mod", "module")
  self.music:looped(true)
  self.music:play()
end

function Main:init()
  Display.palette(PALETTE, Display.PALETTE_BANK_0)
  Display.palette(PALETTE_FONT, Display.PALETTE_BANK_1)
end

function Main:deinit()
end

function Main:update(delta_time)
  self.pool:update(delta_time)

  for _, object in ipairs(self.objects) do
    object:update(delta_time)
  end
end

function Main:render(_)
  local canvas <const> = CANVAS
  local state <const> = STATE

  for _, object in ipairs(self.objects) do
    object:render(canvas, state)
  end
end

return Main
