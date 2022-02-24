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

local Class = require("tofu.core.class")
local System = require("tofu.core.system")
local Canvas = require("tofu.graphics.canvas")
local Display = require("tofu.graphics.display")
local Font = require("tofu.graphics.font")
local Palette = require("tofu.graphics.palette")
local Program = require("tofu.graphics.program")

local SHADES <const> = 64 - 1

local Main = Class.define()

function Main:__ctor()
  local palette = Palette.default("famicube")
  Display.palette(palette)

  local canvas = Canvas.default()
  local image = canvas:image()
  local width, _ = image:size()
  local half_width = width * 0.5

  self.font = Font.default(0, 1)

  local step = 255 / SHADES

  local program = Program.new()
  program:wait(0, 0)
  program:color(0, 0x00, 0x00, 0x00)
  program:wait(0, 0)
  for i = SHADES, 0, -1 do
    local v = step * (SHADES - i)
    print(v)
    program:color(0, v, 0x00, 0x00)
    program:skip(half_width, 0) -- Wait half-width on the same raster-line...
    program:color(0, 0x00, 0x00, v)
    program:skip(-half_width, 1) --- ... then rewind back to the beginning of the next line.
  end
  for i = 0, SHADES do
    local v = step * (SHADES - i)
    program:color(0, v, 0x00, v)
    program:skip(half_width, 0)
    program:color(0, 0x00, v, 0x00)
    program:skip(-half_width, 1)
  end
  program:color(0, 0x00, 0x00, 0x00)
  self.program = program
end

function Main:process()
end

function Main:update(_)
  local canvas = Canvas.default()
  local image = canvas:image()
  local _, height = image:size()

  local t = System.time() * 2.5
  local y = math.sin(t) * height * 0.25 + height * 0.5
  self.program:wait(0, y - SHADES, 2)

  Display.program(self.program)
end

function Main:render(_)
  local canvas = Canvas.default()
  local image = canvas:image()
  image:clear(0)

  canvas:write(0, 0, self.font, string.format("FPS: %d", System.fps()))
end

return Main