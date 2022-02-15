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
local Input = require("tofu.events.input")
local Canvas = require("tofu.graphics.canvas")
local Display = require("tofu.graphics.display")
local Font = require("tofu.graphics.font")
local Image = require("tofu.graphics.image")
local Palette = require("tofu.graphics.palette")
local Program = require("tofu.graphics.program")

local Main = Class.define()

local GRADIENTS <const> = 32
local STEP <const> = 2
local FONT_INDEX <const> = 1

local LINES <const> = {
  "Hello, World!",
  "",
  "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~",
  "Welcome to Tofu Engine!",
  "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~",
  "",
  "Hello, World! #04",
  "Hello, World! #05",
  "Hello, World! #06",
  "Hello, World! #07",
  "Hello, World! #08",
  "Hello, World! #09",
  "Hello, World! #10",
  "Hello, World! #11",
  "Hello, World! #12",
  "Hello, World! #13",
  "Hello, World! #14",
  "Hello, World! #15",
  "Hello, World! #16",
  "Hello, World! #17",
  "Hello, World! #18",
  "Hello, World! #19",
  "Hello, World! #20",
  "Hello, World! #21",
  "Hello, World! #22",
}

local SPEED_IN_PIXELS_PER_SECOND <const> = -16.0
local FONT_HEIGHT <const> = 8
local FONT_WIDTH <const> = 8

function Main:__ctor()
  local palette <const> = Palette.default("famicube")
  Display.palette(palette)

  local canvas <const> = Canvas.default()
  local _, height <const> = canvas:image():size()
  canvas:transparent({ [0] = false, [63] = true })

  local program = Program.new()
  for i = 0, GRADIENTS do
    local y <const> = i * STEP
    program:wait(0, y)
    local ratio = i / GRADIENTS
    local v = math.tointeger(255.0 * ratio)
    program:color(FONT_INDEX, v, v, v)
  end
  for i = GRADIENTS, 0, -1 do
    local y <const> = height - STEP - i * STEP
    program:wait(0, y)
    local ratio = i / GRADIENTS
    local v = math.tointeger(255.0 * ratio)
    program:color(FONT_INDEX, v, v, v)
  end
  Display.program(program)

  self.font = Font.new(Image.new("assets/images/font-8x8.png", 63, FONT_INDEX), FONT_WIDTH, FONT_HEIGHT)
  self.offset = height
  self.current = 1
  self.running = true
end

function Main:process()
  if Input.is_pressed("select") then
    self.running = not self.running
  end
end

function Main:update(delta_time)
  if not self.running then
    return
  end

  self.offset = self.offset + SPEED_IN_PIXELS_PER_SECOND * delta_time
  if self.offset <= -FONT_HEIGHT then
    self.offset = self.offset + FONT_HEIGHT
    self.current = self.current + 1
  end
end

function Main:render(_)
  local canvas = Canvas.default()
  local image = canvas:image()
  image:clear(0)

  local width <const>, height <const> = image:size()

  local x <const> = width * 0.5
  local y <const> = self.offset
  local lines_on_screen <const> = math.tointeger(math.ceil((height - self.offset) / FONT_HEIGHT))
  local lines_available <const> = #LINES - self.current
  local lines_to_render <const> = math.min(lines_available, lines_on_screen)

  for i = 0, lines_to_render do
    local index <const> = self.current + i
    canvas:write(x, y + i * FONT_HEIGHT, self.font, LINES[index], "center", "top")
  end

  canvas:push()
    canvas:shift(FONT_INDEX, 31)
    canvas:write(0, 0, self.font, System.fps())
  canvas:pop()
end

return Main
