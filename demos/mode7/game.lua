local System = require("tofu.core").System
local Surface = require("tofu.graphics").Surface
local Canvas = require("tofu.graphics").Canvas
local Font = require("tofu.graphics").Font
local Input = require("tofu.events").Input
local Class = require("tofu.util").Class

local Game = Class.define()

function Game:__ctor()
  Canvas.palette("gameboy")
  Canvas.background(0)

  self.surface = Surface.new("assets/map.png")
  self.font = Font.default(15, 3)
  self.time = 0
  self.speed = 1.0
  self.running = true
end

function Game:input()
  if Input.is_key_pressed(Input.LEFT) then
    self.speed = self.speed * 0.5
  elseif Input.is_key_pressed(Input.RIGHT) then
    self.speed = self.speed * 2.0
  elseif Input.is_key_pressed(Input.DOWN) then
    self.speed = 1.0
  elseif Input.is_key_pressed(Input.Y) then
    self.running = not self.running
  end
end

function Game:update(delta_time)
  self.time = self.time + delta_time * self.speed
  if not self.running then
    return
  end
  local t = self.time
  local cos, sin = math.cos(t), math.sin(t)
  local tx, ty = (math.cos(t) + 1) * 0.5 * self.surface:width(), 0
  local s = (math.sin(t) + 1) * 0.5 * 3 + 0.125
  local x0 = tx
  local y0 = ty
  local a, b = cos * s, sin
  local c, d = -sin, cos * s
  self.surface:transformation(x0, y0, a, b, c, d)
end

function Game:render(_)
  Canvas.clear()

  self.surface:blit(128, 128)

  self.font:write(string.format("FPS: %d", System.fps()), 0, 0, "left")
end

return Game