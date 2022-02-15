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

--[[
- was about to ditch primitives from the engine, probably just keep the lines for debug purposes
- asked on twitter
- quite a few replies, some examples (UI, debug, even games)
- it's not a core feature, nothing too performant
- but Jett came with an example: LASERS!!!
- implemented them, soo coool
- ended in implementing blending modes too

- also, reworked canvas.
]]

local Class = require("tofu.core.class")
local System = require("tofu.core.system")
local Input = require("tofu.events.input")
local Canvas = require("tofu.graphics.canvas")
local Display = require("tofu.graphics.display")
local Font = require("tofu.graphics.font")
local Palette = require("tofu.graphics.palette")

local PALETTE <const> = Palette.default("famicube")
local FOREGROUND <const> = PALETTE:match(255, 255, 255)

local MIN_DISTANCE <const> = 8

local Main = Class.define()

function Main:__ctor()
  Display.palette(PALETTE)

  local canvas = Canvas.default()
  canvas:transparent(0, false)

  local width, height = canvas:image():size()

  Input.cursor_area(0, 0, width, height) -- FIXME: painful!

  self.font = Font.default(0, FOREGROUND)
  self.colors = {
      PALETTE:match( 15,  15,  15),
      PALETTE:match( 31,  31,  31),
      PALETTE:match( 31,  31,  63),
      PALETTE:match( 63,  63, 127),
      PALETTE:match(191, 191, 223)
    }
  self.step = 4
  self.lines = {}
  self.a = { x = width * 0, y = height * 0.5 }
  self.b = { x = width - 1, y = height * 0.5 }
  self.c = { x = width * 0.5, y = height * 0.5 }
  self.changed = true
end

function Main:process()
  local cx, cy = Input.cursor()
  self.c.x = cx
  self.c.y = cy

  if Input.is_down("a") then
    self.a.x = cx
    self.a.y = cy
    self.changed = true
  elseif Input.is_down("b") then
    self.b.x = cx
    self.b.y = cy
    self.changed = true
  end

  if self.changed then
    local dx = self.b.x - self.a.x
    local dy = self.b.y - self.a.y
    local d = ((dx * dx) + (dy * dy)) ^ 0.5
    if d >= MIN_DISTANCE then
      self.v = { x = dx / d, y = dy / d }
      self.d = d
    end
  end
end

function Main:update(_) -- delta_time
  local lines = {}
  for index, color in ipairs(self.colors) do
    local points = {}

    local span = (#self.colors + 1 - index) * 4

    local segments = math.tointeger(math.ceil(self.d / (self.step + math.random(0, 3))))
    local step = self.d / segments
    for i = 0, segments do
      local d = i * step

      local x = self.a.x + self.v.x * d
      local y = self.a.y + self.v.y * d

      local r = (d / self.d - 0.5) * 2
      local o = math.random(-span, span) * (1 - math.abs(r))
      local ox = self.v.y * o
      local oy = -self.v.x * o

      table.insert(points, x + ox)
      table.insert(points, y + oy)
    end
    table.insert(lines, { color = color, points = points })
  end
  self.lines = lines
end

function Main:render(_) -- ratio
  local canvas = Canvas.default()
  local image = canvas:image()
  image:clear(0)

  for _, line in ipairs(self.lines) do
    canvas:polyline(line.points, line.color)
  end

  canvas:square("fill", self.c.x - 1, self.c.y - 1, 3, FOREGROUND)

  canvas:write(0, 0, self.font, string.format("FPS: %d", System.fps()))
end

return Main
