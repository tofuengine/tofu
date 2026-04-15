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
local Font <const> = require("tofu.graphics.font")

local PALETTE <const> = Palette.new({ { 0, 0, 0 }, { 255, 0, 0 } }) -- Red on black.
local FONT <const> = Font.default()
local CANVAS <const> = Canvas.default()
local WIDTH <const>, HEIGHT <const> = CANVAS:image():size()

local LOGO <const> = "Tofu Engine"

local FONT_INDEX <const> = 0

local BACKGROUND_INDEX <const> = 0
local FOREGROUND_INDEX <const> = 1

-- TODO: implement "splash" state that emulates Amiga's boot-splash.

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

function Splash:render(canvas, _)
  canvas:clear(BACKGROUND_INDEX)

  canvas:push()
    canvas:shift(FONT_INDEX, FOREGROUND_INDEX)
    canvas:write(WIDTH / 2, HEIGHT / 2, FONT, LOGO, "center", "middle")
  canvas:pop()
end

function Splash.is_done()
  return System.time() > 3.0
end

return Splash
