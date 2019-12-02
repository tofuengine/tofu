local System = require("tofu.core").System
local Canvas = require("tofu.graphics").Canvas
local Font = require("tofu.graphics").Font
local Input = require("tofu.events").Input
local Class = require("tofu.util").Class

local Main = Class.define()

local PALETTE = {
    0xFF000000, 0xFF240000, 0xFF480000, 0xFF6D0000,
    0xFF910000, 0xFFB60000, 0xFFDA0000, 0xFFFF0000,
    0xFFFF3F00, 0xFFFF7F00, 0xFFFFBF00, 0xFFFFFF00,
    0xFFFFFF3F, 0xFFFFFF7F, 0xFFFFFFBF, 0xFFFFFFFF
  }

function Main:__ctor()
  Canvas.palette(PALETTE) -- "arne-16")
  Canvas.palette("arne-16")

  self.font = Font.default(0, 1)
  self.mode = 0
end

function Main:input()
  if Input.is_key_pressed(Input.START) then
    System.quit()
  elseif Input.is_key_pressed(Input.RIGHT) then
    self.mode = (self.mode % 10) + 1
  elseif Input.is_key_pressed(Input.LEFT) then
    self.mode = ((self.mode + 8) % 10) + 1
  end
end

function Main:update(_) -- delta_time
end

function Main:render(_) -- ratio
  Canvas.clear()

  if self.mode == 0 then
    local cx, cy = 8, 32
    for r = 0, 12 do
      Canvas.circle("fill", cx, cy, r, 1)
      Canvas.circle("line", cx, cy + 64, r, 1)
      cx = cx + 2 * r + 8
    end

    Canvas.polyline({ 64, 64, 64, 128, 128, 128 }, 2)

    local x0 = (math.random() * Canvas.width() * 2) - Canvas.width() * 0.5
    local y0 = (math.random() * Canvas.width() * 2) - Canvas.width() * 0.5
    local x1 = (math.random() * Canvas.width() * 2) - Canvas.width() * 0.5
    local y1 = (math.random() * Canvas.width() * 2) - Canvas.width() * 0.5
    Canvas.line(x0, y0, x1, y1, 3)
  elseif self.mode == 1 then
    local dx = math.cos(System.time()) * 32
    local dy = math.sin(System.time()) * 32
    Canvas.circle("fill", 128, 64, 32, 1)
    Canvas.line(128, 64, 128 + dx, 64 + dy, 2)
  elseif self.mode == 2 then
    Canvas.triangle("fill", 5, 50, 5, 150, 150, 150, 1)
    Canvas.triangle("fill", 5, 50, 150, 50, 150, 150, 3)
  elseif self.mode == 3 then
    local x0 = ((math.cos(System.time() * 0.125) + 1.0) * 0.5) * Canvas.width()
    local y0 = ((math.cos(System.time() * 0.342) + 1.0) * 0.5) * Canvas.height()
    local x1 = ((math.sin(System.time() * 0.184) + 1.0) * 0.5) * Canvas.width()
    local y1 = ((math.sin(System.time() * 0.223) + 1.0) * 0.5) * Canvas.height()
    local x2 = ((math.cos(System.time() * 0.832) + 1.0) * 0.5) * Canvas.width()
    local y2 = ((math.sin(System.time() * 0.123) + 1.0) * 0.5) * Canvas.height()
    Canvas.triangle("fill", x0, y0, x1, y1, x2, y2, 2)
    Canvas.triangle("line", x0, y0, x1, y1, x2, y2, 7)
  elseif self.mode == 4 then
    local x = ((math.cos(System.time() * 0.125) + 1.0) * 0.5) * Canvas.width()
    local y = ((math.cos(System.time() * 0.342) + 1.0) * 0.5) * Canvas.height()
    Canvas.square("fill", x, y, 75, 2)
    Canvas.square("line", 96, 96, 64, 2)
  elseif self.mode == 5 then
    local cx = Canvas.width() * 0.5
    local cy = Canvas.height() * 0.5
    Canvas.circle("fill", cx, cy, 50, 3)
    Canvas.circle("line", cx, cy, 50, 4)
  elseif self.mode == 6 then
    local cx = Canvas.width() * 0.5
    local cy = Canvas.height() * 0.5
    Canvas.circle("line", cx, cy, 50, 4)
    Canvas.circle("fill", cx, cy, 50, 3)
  elseif self.mode == 7 then
    local cx = ((math.cos(System.time() * 0.125) + 1.0) * 0.5) * Canvas.width()
    local cy = ((math.cos(System.time() * 0.342) + 1.0) * 0.5) * Canvas.height()
    local r = ((math.sin(System.time() * 0.184) + 1.0) * 0.5) * 63 + 1
    Canvas.circle("fill", cx, cy, r, 6)
  elseif self.mode == 8 then
    local cx = ((math.cos(System.time() * 0.125) + 1.0) * 0.5) * Canvas.width()
    local cy = ((math.cos(System.time() * 0.342) + 1.0) * 0.5) * Canvas.height()
    local r = ((math.sin(System.time() * 0.184) + 1.0) * 0.5) * 63 + 1
    Canvas.circle("line", cx, cy, r, 7)
  elseif self.mode == 9 then
    local colors = { 13, 11, 9, 7, 5, 3, 1 }
    local y = (math.sin(System.time()) + 1.0) * 0.5 * Canvas.height()
    Canvas.hline(0, y, Canvas.width() - 1, 15)
    for i, c in ipairs(colors) do
      Canvas.hline(0, y - i, Canvas.width() - 1, c)
      Canvas.hline(0, y + i, Canvas.width() - 1, c)
    end
  elseif self.mode == 10 then
    Canvas.point(4, 4, 1)
    Canvas.line(8, 8, 32, 32, 2)
    Canvas.rectangle("line", 4, 23, 8, 8, 3)
    Canvas.triangle("line", 150, 150, 50, 250, 250, 250, 3)
    Canvas.rectangle("fill", 4, 12, 8, 8, 3)
  end

  self.font:write(string.format("FPS: %d", System.fps()), 0, 0, "left")
  self.font:write(string.format("mode: %d", self.mode), Canvas.width(), 0, "right")
end

return Main
