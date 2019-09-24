local System = require("tofu.core").System
local Canvas = require("tofu.graphics").Canvas
local Font = require("tofu.graphics").Font
local Input = require("tofu.events").Input
local Class = require("tofu.util").Class

local Game = Class.define()

function Game:__ctor()
  Canvas.palette("arne-16")

  self.font = Font.default(0, 1)
  self.mode = 0
end

function Game:input()
  if Input.is_key_pressed(Input.START) then
    System.quit()
  elseif Input.is_key_pressed(Input.X) then
    self.mode = (self.mode % 10) + 1
  end
end

function Game:update(delta_time)
end

function Game:render(ratio)
  Canvas.clear()

  if self.mode == 0 then
    local dx = math.cos(System.time()) * 32
    local dy = math.sin(System.time()) * 32
    Canvas.circle("fill", 128, 64, 32, 1)
    Canvas.line(128, 64, 128 + dx, 64 + dy, 2)
  elseif self.mode == 1 then
    Canvas.triangle("fill", 5, 50, 5, 150, 150, 150, 1)
    Canvas.triangle("fill", 5, 50, 150, 50, 150, 150, 3)
    local x0 = ((math.cos(System.time() * 0.125) + 1.0) * 0.5) * Canvas.width()
    local y0 = ((math.cos(System.time() * 0.342) + 1.0) * 0.5) * Canvas.width()
    local x1 = ((math.sin(System.time() * 0.184) + 1.0) * 0.5) * Canvas.width()
    local y1 = ((math.sin(System.time() * 0.223) + 1.0) * 0.5) * Canvas.width()
    local x2 = ((math.cos(System.time() * 0.832) + 1.0) * 0.5) * Canvas.width()
    local y2 = ((math.sin(System.time() * 0.123) + 1.0) * 0.5) * Canvas.width()
    Canvas.triangle("fill", x0, y0, x1, y1, x2, y2, 2)
    Canvas.triangle("line", x0, y0, x1, y1, x2, y2, 7)
  elseif self.mode == 2 then
    Canvas.square("fill", 200, 10, 75, 2)
    Canvas.square("line", 96, 96, 64, 2)
  elseif self.mode == 3 then
    Canvas.circle("fill", 200, 100, 50, 1)
    Canvas.circle("line", 100, 100, 50, 2)
  elseif self.mode == 4 then
  elseif self.mode == 5 then
  elseif self.mode == 6 then
  elseif self.mode == 7 then
  elseif self.mode == 8 then
    Canvas.point(4, 4, 1)
    Canvas.line(8, 8, 32, 32, 2)
    Canvas.rectangle("line", 4, 23, 8, 8, 3)
  elseif self.mode == 9 then
    Canvas.triangle("line", 150, 150, 50, 250, 250, 250, 3)
    Canvas.rectangle("fill", 4, 12, 8, 8, 3)
  end

  self.font:write(string.format("FPS: %d", System.fps()), 0, 0, "left")
  self.font:write(string.format("mode: %d", self.mode), Canvas.width(), 0, "right")
end

return Game
