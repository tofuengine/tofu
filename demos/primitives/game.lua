local System = require("tofu.core").System
local Canvas = require("tofu.graphics").Canvas
local Font = require("tofu.graphics").Font
local Input = require("tofu.events").Input
local Class = require("tofu.util").Class

local Game = Class.define()

function Game:__ctor()
  Canvas.palette("arne-16")

  self.font = Font.default(0, 1)
  self.segments = 30
end

function Game:input()
  if Input.is_key_pressed(Input.START) then
    System.quit()
  elseif Input.is_key_pressed(Input.LEFT) then
    self.segments = self.segments - 1
  elseif Input.is_key_pressed(Input.RIGHT) then
    self.segments = self.segments + 1
  end
end

function Game:update(delta_time)
end

function Game:render(ratio)
  Canvas.clear()

  Canvas.point(4, 4, 1)
  Canvas.line(4, 8, 7, 8, 2)
--Canvas.triangle("fill", 150, 150, 50, 250, 250, 250, 3)
  Canvas.rectangle("fill", 4, 12, 8, 8, 3)
  Canvas.rectangle("line", 4, 23, 8, 8, 3)
--Canvas.square("fill", 200, 10, 75, 2)
--Canvas.circle("line", 100, 100, 50, 2)
--Canvas.circle("fill", 200, 100, 50, 1)
--Canvas.circle("fill", 300, 100, 50, 1, _segments)

  self.font:write(string.format("FPS: %d", System.fps()), Canvas.width(), 0, "right")
end

return Game
