--[[
MIT License

Copyright (c) 2019-2022 Marco Lizza

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
local Controller = require("tofu.input.controller")
local Canvas = require("tofu.graphics.canvas")
local Display = require("tofu.graphics.display")
local Font = require("tofu.graphics.font")
local Image = require("tofu.graphics.image")
local Palette = require("tofu.graphics.palette")
local XForm = require("tofu.graphics.xform")

-- The entry point is a class, we are creating with a helper function.
local Main = Class.define()

-- The message we are displaying, as a "constant".
local MESSAGE <const> = "Hello, Tofu!"

function Main:__ctor()
  -- Load a predefined palette, we choose Pico-8's one.
  Display.palette(Palette.default("pico-8"))

  self.factor = 0.0

  -- Get the default image size. The image is bound the default canvas:
  local canvas = Canvas.default()
  local image = canvas:image()
  local width, height = image:size()

  -- Load a custom 8x8 font from file, setting palette colour `0` as background
  -- and `15` as foreground.
  -- Please note that, as default, palette colour `0` is set as transparent. This
  -- means that the font background colour won't be drawn.
  self.canvas = Canvas.new(Image.new(width, height))
  self.font = Font.new(Image.new("assets/font-8x8.png", 0, 15), 8, 8)
  self.font_digits = Font.new(Image.new("assets/digits.png", 0, 15), 8, 8, "0123456789")

  self.xform = XForm.new() -- TODO: pass clamp mode?
  self.xform:wrap("border")
  self.xform:matrix(1, 0, 0, 1, width * 0.5, height * 0.5)
  self.xform:warp(height, self.factor)
end

function Main:process()
  local recompute = false

  local controller = Controller.default()
  if controller:is_pressed("start") then
    self.running = not self.running
  elseif controller:is_pressed("y") then
    self.factor = self.factor + 0.1
    recompute = true
  elseif controller:is_pressed("x") then
    self.factor = self.factor - 0.1
    recompute = true
  end

  if recompute then
    local _, height = self.canvas:image():size()
    self.xform:warp(height, self.factor)
  end
end

function Main:update(_)
end

function Main:render(_)
  -- Query current time since the start, expressed in seconds (as a floating point number).
  local t = System.time()

  -- Get a reference to the off-screen canvas (i.e. the the virtual-screen).
  local canvas = self.canvas

  -- Clear the virtual-screen with default background colour (i.e. palette colour #0).
  local image = canvas:image()
  image:clear(0)

  -- Query for text width/height and calculate the (screen-centred) origin
  -- x/y position.
  local canvas_width, canvas_height = image:size()
  local text_width, text_height = self.font:size(MESSAGE)
  local x, y = (canvas_width - text_width) * 0.5, (canvas_height - text_height) * 0.5

  canvas:rectangle("fill", 2, 2, canvas_width - 4, canvas_height - 4, 3)

  -- Scan the message text one char at time. We need the current char index in order
  -- to change color for each character.
  for i = 1, #MESSAGE do
    -- Get the i-th string character.
    local c = MESSAGE:sub(i, i)

    -- Query for font char width/height, we'll use it for offsetting the characters.
    local char_width, char_height = self.font:size(c)

    -- Compute the vertical offset using a sine wave, each character with a different value.
    local dx = math.cos(t * 1.5           ) * 2 * char_width + (i - 1) * char_width
    local dy = math.sin(t * 2.5 + i * 0.25) * 2 * char_height

    -- Convert the time to an integer (speeding it up a bit) and get a different
    -- colour for each character. Then instruct the engine that colour `15` need to be
    -- remapped to colour `index`.
    local index = (tonumber(t * 5) + i) % 16
    canvas:shift(15, index)

    -- Draw the i-th character, accounting for vertical offset.
    canvas:write(x + dx, y + dy, self.font, c)
  end

  canvas:write(0, 0, self.font_digits, "9876543210")

  -- Transfer to the virtual-screen canvas through transformation.
  self:_flip()
end

function Main:_flip()
  local canvas = Canvas.default()
  local image = canvas:image()
  image:clear(0)

  canvas:xform(self.canvas:image(), self.xform)
end

return Main
