--[[
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
local System = require("tofu.core.system")
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

  -- Load a custom 8x8 font from file, setting palette colour `0` as background
  -- and `15` as foreground.
  -- Please note that, as default, palette colour `0` is set as transparent. This
  -- means that the font background colour won't be drawn.
  self.font = Font.from_image("assets/font-8x8.png", 8, 8, 0, 15)
end

function Main:process()
  -- Nothing to do, here.
end

function Main:update(_)
  -- Ditto.
end

function Main:render(_)
  -- Get a reference to the default canvas (i.e. the the virtual-screen).
  local canvas = Canvas.default()

  -- Query current time since the start, expressed in seconds (as a floating point number).
  local t = System.time()

  -- Clear the virtual-screen with default background colour (i.e. palette colour #0).
  canvas:clear(0)

  -- Query for text width/height and calculate the (screen-centred) origin
  -- x/y position.
  local canvas_width, canvas_height = self.canvas:size()
  local text_width, text_height = self.font:size(MESSAGE)
  local x, y = (canvas_width - text_width) * 0.5, (canvas_height - text_height) * 0.5

  -- Scan the message text one char at time. We need the current char index in order
  -- to change color for each character.
  for i = 1, #MESSAGE do
    -- Get i-th string character.
    local c = MESSAGE:sub(i, i);

    -- Query for current char width/height, we'll use it for offsetting the characters.
    local char_width, char_height = self.font:size(c)

    -- Compute the vertical offset using a sine wave, each character with a different value.
    local dx = (i - 1) * char_width
    local dy = math.sin(t * 2.5 + dx * 0.05) * char_height

    -- Convert the time to an integer (speeding it up a bit) and get a different
    -- colour for each character. Then instruct the engine that colour `15` need to be
    -- remapped to colour `index`.
    local index = (tonumber(t * 5) + i) % 16
    canvas:shift(15, index)

    -- Draw the i-th character, accounting for vertical offset.
    self.font:write(c, x + dx, y + dy)
  end
end

return Main
