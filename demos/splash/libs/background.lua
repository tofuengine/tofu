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

local Class = require("tofu.core").Class
local System = require("tofu.core").System
local Canvas = require("tofu.graphics").Canvas
local Display = require("tofu.graphics").Display
local Program = require("tofu.graphics").Program
local Font = require("tofu.graphics").Font
local Timer = require("tofu.timers").Timer

local Background = Class.define()

function Background:__ctor(canvas, transparent, palette)
  local _, height = canvas:size()
  local half_height = math.tointeger(height * 0.5)
  local quarter_height = math.tointeger(height * 0.25)

  self.font = Font.new(Canvas.new("assets/images/font-8x8.png", transparent, palette:color_to_index(255, 255, 255)),
      8, 8)

  self.timer = Timer.new(10, 0, function(_)
      local program = Program.gradient(transparent, {
          { 0, palette:index_to_color(math.random(0, transparent)) },
          { quarter_height - 1, palette:index_to_color(math.random(0, transparent)) },
          { half_height - 1, palette:index_to_color(math.random(0, transparent)) },
          { height - quarter_height - 1, palette:index_to_color(math.random(0, transparent)) },
          { height - 1, palette:index_to_color(math.random(0, transparent)) }
        })
--      program:wait(0, height - math.tointeger(quarter_height / 2) - 1)
--      program:modulo(-width * 4)
      Display.program(program)
    end)
end

function Background:update(_)
end

function Background:render(canvas)
--[[
  self.canvas:push()
  local x, y = 0, 0
  local dy = 0
  local message = '* T O F U - E N G I N E '
  local colours = { 0, 5, 6, 10, 7, 23, 6, 5 }
  local offset = 0 -- math.tointeger(t * 17.0)
  local cursor = math.tointeger(t * 5.0)
  for k = 0, 49 do
    local dx = 0
    local max_char_height = 0
    for i = 0, 59 do
      local j = ((cursor + k + i) % #message) + 1
      local c = message:sub(j, j)
      local char_width, char_height = self.font:size(c)
      if char_height > max_char_height then
        max_char_height = char_height
      end
      canvas:shift(0, colours[(i + offset) % #colours + 1])
      self.font:write(c, x + dx, y + dy)
      dx = dx + char_width
    end
    dy = dy + char_height
  end
  self.canvas:pop()
--]]
  local width, height = canvas:size()

  self.font:write(canvas, width, 0, string.format("%d FPS", System.fps()), "right", "top")
  self.font:write(canvas, width, height, string.format("%d KiB", System.heap("kb")), "right", "bottom")
end

return Background
