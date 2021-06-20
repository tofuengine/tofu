--[[
MIT License

Copyright (c) 2019-2021 Marco Lizza

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
local Class = require("tofu.core").Class
local System = require("tofu.core").System
local Canvas = require("tofu.graphics").Canvas
local Display = require("tofu.graphics").Display
local Font = require("tofu.graphics").Font
local Palette = require("tofu.graphics").Palette

-- The entry point is a class, we are creating with a helper function.
local Main = Class.define()

-- The message we are displaying, as a "constant".
local MESSAGE = "Hello, Tofu!"

function Main:__ctor()
  -- Load a predefined palette, we choose Pico-8's one.
  Display.palette(Palette.new("pico-8"))

  -- Create a default font, palette colour `0` as background and `15` as foreground.
  -- Please note that, as default, palette colour `0` is set as transparent. This
  -- means that the font background colour won't be drawn.
  self.font = Font.default(0, 15)
end

function Main:input()
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
  canvas:clear()

  -- Query for text width/height and calculate the (screen-centred) origin
  -- x/y position.
  local canvas_width, canvas_height = self.canvas:size()
  local text_width, text_height = self.font:size(MESSAGE)
  local x, y = (canvas_width - text_width) * 0.5, (canvas_height - text_height) * 0.5

  -- Scan the message text one char at time.
  for c in MESSAGE:gmatch(".") do
    -- Query for current char width/height, we'll use it for offsetting the characters.
    local char_width, char_height = self.font:size(c)

    -- Compute the vertical offset using a sine wave, each character with a different value.
    local dy = math.sin(t * 2.5 + x * 0.05) * char_height

    -- Draw the character, accounting for vertical offset.
    self.font:write(c, x, y + dy)

    -- Move to the right the drawing position by the character width amount.
    x = x + char_width
  end
end

return Main