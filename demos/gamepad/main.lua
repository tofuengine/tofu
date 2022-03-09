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
local Bank = require("tofu.graphics.bank")
local Canvas = require("tofu.graphics.canvas")
local Image = require("tofu.graphics.image")
local Display = require("tofu.graphics.display")
local Palette = require("tofu.graphics.palette")
local Font = require("tofu.graphics.font")
local Controller = require("tofu.input.controller")
local Cursor = require("tofu.input.cursor")

local Main = Class.define()

local IDS <const> = {
    "a", "b", "x", "y",
    "lb", "rb",
    "up", "down", "left", "right",
    "select", "start"
  }

local INDICES <const> = {
    0, 1, 2, 3,
    4, 5,
    12, 13, 14, 15,
    24, 25
  }

function Main:__ctor()
  Display.palette(Palette.default("pico-8"))

  self.bank = Bank.new(Image.new("assets/sheet.png", 0), 12, 12)
  self.font = Font.default(0, 15)
  self.down = {}
  self.scale = {}
end

function Main:process()
  local controller = Controller.default()

  for _, id in ipairs(IDS) do
    self.down[id] = controller:is_down(id)
    if controller:is_pressed(id) then
      self.scale[id] = 3.0
    end
  end
end

function Main:update(delta_time)
  for _, id in ipairs(IDS) do
    if self.scale[id] and self.scale[id] > 1.0 then
      self.scale[id] = math.max(1.0, self.scale[id] - delta_time * 12.0)
    end
  end
end

local function draw_stick(canvas, cx, cy, radius, _, _, angle, magnitude, pressed)
  local dx, dy = math.floor(math.cos(angle) * magnitude * radius + 0.5),
                 math.floor(math.sin(angle) * magnitude * radius + 0.5)
--  local dx, dy = x * radius, y * radius
  if pressed then
    canvas:circle("fill", cx, cy, radius, 2)
  end
  canvas:circle("line", cx, cy, radius, 1)
  canvas:line(cx, cy, cx + dx, cy + dy, 3)
end

local function draw_trigger(canvas, cx, cy, radius, magnitude)
  if magnitude > 0.0 then
    canvas:circle("fill", cx, cy, magnitude * radius, 2)
  end
  canvas:circle("line", cx, cy, radius, 1)
end

function Main:render(_)
  local t = System.time()

  local canvas = Canvas.default()
  local image = canvas:image()
  image:clear(0)

  local cw, ch = self.bank:size(Bank.NIL)
  local width, height = image:size()

  local x, y = (width - #IDS * cw) * 0.5, (height - ch) * 0.5
  for index, id in ipairs(IDS) do
    local dy = math.sin(t * 2.5 + x * 0.5) * ch
    if self.down[id] then
      dy = 0
    end
    local ox, oy = 0, 0
    local s = 1.0
    if self.scale[id] then
      s = self.scale[id]
      ox = (cw * s - cw) * 0.5
      oy = (ch * s - ch) * 0.5
    end
    canvas:sprite(x - ox, y - oy + dy, self.bank, INDICES[index], s, s)
    x = x + cw
  end

  local controller = Controller.default()
  local cy = height * 0.5
  local lx, ly, la, lm = controller:stick("left")
  local rx, ry, ra, rm = controller:stick("right")
  draw_stick(canvas, 24, cy - 12, 8, lx, ly, la, lm, controller:is_down("lt"))
  draw_stick(canvas, 232, cy - 12, 8, rx, ry, ra, rm, controller:is_down("rt"))
  local tl, tr = controller:triggers()
  draw_trigger(canvas, 24, cy + 12, 8, tl)
  draw_trigger(canvas, 232, cy + 12, 8, tr)

  local cursor = Cursor.default()
  local mx, my = cursor:position()
  canvas:line(mx - 3, my, mx - 1, my, 2)
  canvas:line(mx + 1, my, mx + 3, my, 2)
  canvas:line(mx, my - 3, mx, my - 1, 2)
  canvas:line(mx, my + 1, mx, my + 3, 2)

  canvas:write(0, 0, self.font, string.format("FPS: %d", System.fps()))
  canvas:write(width, height, self.font, string.format("X:%.2f Y:%.2f A:%.2f M:%.2f", lx, ly, la, lm),
    "right", "bottom")
end

return Main
