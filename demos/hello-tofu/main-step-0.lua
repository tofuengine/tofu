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

-- Include the modules we'll be using.
local Class = require("tofu.core.class")
local Canvas = require("tofu.graphics.canvas")
local Display = require("tofu.graphics.display")
local Font = require("tofu.graphics.font")
local Palette = require("tofu.graphics.palette")

-- The entry point is a class, we are creating with a helper function.
local Main = Class.define()

-- The message we are displaying, as a "constant".
local MESSAGE = "Hello, Tofu!"

function Main:__ctor()
  -- Load a predefined palette, we choose Pico-8's one.
  Display.palette(Palette.default("pico-8"))

  -- Create a default font, palette colour `0` as background and `15` as foreground.
  -- Please note that, as default, palette colour `0` is set as transparent. This
  -- means that the font background colour won't be drawn.
  self.font = Font.default(0, 15)
end

function Main:init()
end

function Main:update(_)
  -- Nothing to do, here.
end

function Main:render(_)
  -- Get a reference to the default canvas (i.e. the the virtual-screen)...
  local canvas = Canvas.default()

  -- ... and clear it w/ default background palette colour (i.e. palette index #0).
  canvas:clear(0)

  -- Get the canvas width and height.
  local canvas_width, canvas_height = canvas:size()

  -- We need the font (message) width and height to center it on screen.
  local text_width, text_height = self.font:size(MESSAGE)

  -- Compute vertical and horizontal position for the text.
  local x = (canvas_width - text_width) * 0.5
  local y = (canvas_height - text_height) * 0.5

  -- Finally, draw the message on-screen at the given position.
  self.font:write(MESSAGE, x, y)
end

return Main