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
local System <const> = require("tofu.core.system")
local Canvas <const> = require("tofu.graphics.canvas")
local Display <const> = require("tofu.graphics.display")
local Palette <const> = require("tofu.graphics.palette")

local CANVAS <const> = Canvas.default()
local WIDTH <const>, HEIGHT <const> = CANVAS:image():size()
local STATE <const> = CANVAS:state()
local PALETTE <const> = Palette.default('pico-8')

local COLORS <const> = PALETTE:size()
local AMOUNT <const> = COLORS * 2
local HALF_AMOUNT <const> = COLORS

local BACKGROUND_INDEX <const> = 0

local DURATION <const> = 1.0

local Splash <const> = Class.define()

function Splash:__ctor()
end

function Splash:init()
  Display.palette(PALETTE)
end

function Splash:deinit()
end

function Splash:update(_)
end

function Splash:render(_)
  local canvas = CANVAS

  canvas:clear(BACKGROUND_INDEX)

  local step <const> = (System.time() / DURATION) * AMOUNT
  local progress
  if step < HALF_AMOUNT then
    progress = math.floor(step)
  else
    progress = AMOUNT - math.ceil(step)
  end

  local width = WIDTH / COLORS -- TODO: this can be precomputed
  local height = HEIGHT / COLORS

  local y = 0
  for i = 0, COLORS - 1 do
    local x = 0
    for j = 0, AMOUNT - 1 do
      local index = (i + j + progress) % COLORS
      canvas:rectangle('fill', x, y, width, height, index)

      x = x + width
    end
    y = y + height
  end
end

function Splash.is_done()
  return System.time() >= DURATION
end

return Splash
