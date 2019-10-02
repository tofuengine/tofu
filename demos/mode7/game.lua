local System = require("tofu.core").System
local Surface = require("tofu.graphics").Surface
local Canvas = require("tofu.graphics").Canvas
local Font = require("tofu.graphics").Font
local Input = require("tofu.events").Input
local Class = require("tofu.util").Class

local Game = Class.define()

function Game:__ctor()
  Canvas.palette("arne-32")
  Canvas.background(0)

  self.surface = Surface.new("assets/map.png")
  self.font = Font.default(0, 31)
  self.time = 0
  self.mode = 0
  self.speed = 1.0
  self.running = true

  self.offset_x = 0
  self.offset_y = 0
  self.elevation = 64
  self.horizon = Canvas.height() * 0.5
end

function Game:input()
  if Input.is_key_pressed(Input.DOWN) then
    self.speed = self.speed * 0.5
  elseif Input.is_key_pressed(Input.UP) then
    self.speed = self.speed * 2.0
  elseif Input.is_key_pressed(Input.SELECT) then
    self.speed = 1.0
  elseif Input.is_key_pressed(Input.START) then
    self.running = not self.running
  elseif Input.is_key_pressed(Input.LEFT) then
    self.mode = (self.mode + 9) % 10
  elseif Input.is_key_pressed(Input.RIGHT) then
    self.mode = (self.mode + 1) % 10
  elseif Input.is_key_pressed(Input.X) then
    self.elevation = self.elevation + 8
  elseif Input.is_key_pressed(Input.Y) then
    self.elevation = self.elevation - 8
  elseif Input.is_key_pressed(Input.A) then
    self.horizon = self.horizon + 8
  elseif Input.is_key_pressed(Input.B) then
    self.horizon = self.horizon - 8
  end
end

function Game:update(delta_time)
  if not self.running then
    return
  end
  self.time = self.time + delta_time * self.speed

  if self.mode == 7 then
    self.surface:matrix(0, 0, 1, 0, 0, 1)
  else
    local t = self.time
    local cos, sin = math.cos(t), math.sin(t)
    local sx, sy = 1, 1 --(math.sin(t * 2.3) + 1) * 0.5 * 2 + 0.5, (math.cos(t * 3.2) + 1) * 0.5 * 2 + 0.5
    local x0, y0 = self.surface:width() * 0.5, self.surface:height() * 0.5
--  local a, b = s, 0
--  local c, d = 0, s
    local a, b = cos / sx, sin / sx
    local c, d = -sin / sy, cos / sy
    self.surface:matrix(x0, y0, a, b, c, d)
  end
end

function Game:render(_)
  Canvas.clear()

  if self.mode == 0 then
    self.surface:xform()
  elseif self.mode == 1 then
    self.surface:offset(self.surface:width() / 2, self.surface:height() / 2)
    self.surface:xform()
  elseif self.mode == 2 then
    local t = self.time
    local x, y = (math.sin(t) + 1) * 0.5 * self.surface:width(), (math.cos(t) + 1) * 0.5 * self.surface:height()
    self.surface:offset(x, y)
    self.surface:xform()
  elseif self.mode == 3 then
    local t = self.time
    local elevation = 512
    local horizon = (math.sin(t * 1.2) + 1) * 0.5 * Canvas.height()
    self.surface:offset(0, 0)
    self.surface:projection(true, elevation, horizon)
    self.surface:xform()
  elseif self.mode == 4 then
    local t = self.time
    local elevation = (math.sin(t * 1.2) + 1) * 0.5 * 2048
    local horizon = Canvas.height() * 0.25
    self.surface:offset(0, 0)
    self.surface:projection(true, elevation, horizon)
    self.surface:xform()
  elseif self.mode == 5 then
    self.surface:offset(self.offset_x, self.offset_y)
    self.surface:projection(true, self.elevation, self.horizon)
    self.surface:xform()
  elseif self.mode == 6 then
    local t = self.time
    local offset_x = t * 64
    local offset_y = 0
    self.surface:offset(offset_x, offset_y)
    self.surface:projection(false)
    self.surface:xform()
  elseif self.mode == 7 then
    local t = self.time
    local offset_x = 0
    local offset_y = t * 128
    self.surface:offset(offset_x, offset_y)
    self.surface:projection(true, self.elevation, self.horizon)
    self.surface:xform()
  end

  self.font:write(string.format("FPS: %d", System.fps()), 0, 0, "left")
  self.font:write(string.format("Mode: %d", self.mode), Canvas.width(), 0, "right")
end

return Game