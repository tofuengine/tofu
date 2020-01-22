--[[
MIT License

Copyright (c) 2019-2020 Marco Lizza

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
local System = require("tofu.core").System
local Input = require("tofu.events").Input
local Canvas = require("tofu.graphics").Canvas
local Display = require("tofu.graphics").Display
local XForm = require("tofu.graphics").XForm
local Font = require("tofu.graphics").Font
local Class = require("tofu.util").Class

-- The entry point is a class, we are creating with a helper function.
local Main = Class.define()

-- The message we are displaying, as a "constant".
local MESSAGE = "Hello, Tofu!"

local function build_table(canvas, factor) -- 0.4
  local entries = {}
  for scan_line = 1, canvas:height() do
    local angle = (scan_line / canvas:height()) * math.pi
    local sx = (1.0 - math.sin(angle)) * factor + 1.0
    entries[scan_line] = { y = scan_line - 1, a = sx, b = 0.0, c = 0.0, d = sx }
  end
  return entries
end

function Main:__ctor()
  -- Load a predefined palette, we choose Pico-8's one.
  Display.palette("pico-8")

  self.factor = 0.0

  -- Load a custom 8x8 font from file, setting palette color `0` as background
  -- and `15` as foreground.
  -- Please note that, as default, palette color `0` is set as transparent. This
  -- means that the font background color won't be drawn.
  self.canvas = Canvas.new(Canvas.default():size())
  self.font = Font.new(self.canvas, "assets/font-8x8.png", 8, 8, 0, 15)

  self.xform = XForm.new() -- TODO: pass clamp mode?
  self.xform:clamp("border")
  self.xform:matrix(1, 0, 0, 1, self.canvas:width() * 0.5, self.canvas:height() * 0.5)
  self.xform:table(build_table(self.canvas, self.factor))
end

function Main:input()
  local recompute = false

  if Input.is_pressed("start") then
    self.running = not self.running
  elseif Input.is_pressed("y") then
    self.factor = self.factor + 0.1
    recompute = true
  elseif Input.is_pressed("x") then
    self.factor = self.factor - 0.1
    recompute = true
  end

  if recompute then
    self.xform:table(build_table(self.canvas, self.factor))
  end
end

function Main:update(_)
end

function Main:render(_)
  -- Query current time since the start, expressed in seconds (as a floating point number).
  local t = System.time()

  -- Get a reference to the default canvas (i.e. the the virtual-screen).
  local canvas = self.canvas

  -- Clear the virtual-screen with default background color (i.e. palette color #0).
  canvas:clear()

  --
  canvas:rectangle("fill", 2, 2, canvas:width() - 4, canvas:height() - 4, 3)

  -- Query for text width/height and calculate the (screen-centered) origin
  -- x/y position.
  local text_width, text_height = self.font:size(MESSAGE)
  local x, y = (canvas:width() - text_width) * 0.5, (canvas:height() - text_height) * 0.5

  -- Query for font char width/height, we'll use it for offseting the characters.
  local char_width, char_height = self.font:size()

  -- Scan the message text one char at time. We need the current char index in order
  -- to change color for each character.
  for i = 1, #MESSAGE do
    -- Compute the verical offset using a sine wave, each chacter with a different value.
    local dx = math.cos(t * 1.5           ) * 2 * char_width + (i - 1) * char_width
    local dy = math.sin(t * 2.5 + i * 0.25) * 2 * char_height

    -- Convert the time to an integer (speeding it up a bit) and get a different
    -- color for each character. Then instruct the engine that color `15` need to be
    -- remapped to color `index`.
    local index = (tonumber(t * 5) + i) % 16
    canvas:shift(15, index)

    -- Draw the i-th character, accounting for vertical offset.
    self.font:write(MESSAGE:sub(i, i), x + dx, y + dy)
  end

  -- Transfer to the virtual-screen canvas through transformation.
  Canvas.default():clear()
  self.xform:blit(self.canvas, 0, 0)
end

return Main
