local System = require("tofu.core").System
local Surface = require("tofu.graphics").Surface
local Canvas = require("tofu.graphics").Canvas
local Font = require("tofu.graphics").Font
local Input = require("tofu.events").Input
local Class = require("tofu.util").Class

local Game = Class.define()

local function perspective(regs, scan_line) -- H V A B C D X Y
  local yc = scan_line -- - 128 -- Canvas.height() * 0.25
  if yc <= 0 then
    return regs
  end

  local p = 128 / yc -- (Canvas.height() * 0.25) / yc;

  regs[3] = regs[3] * p
  regs[4] = regs[4] * p
  regs[5] = regs[5] * p
  regs[6] = regs[6] * p

  return regs
end

function Game:__ctor()
  Canvas.palette("arne-32")
  Canvas.background(0)

  self.surface = Surface.new("assets/world.png")
  self.font = Font.default(0, 31)
  self.running = true

  self.x = 0
  self.y = 0
  self.angle = 0
  self.speed = 0.0

  self.surface:projection(perspective)
--  self.surface:projection()
end

function Game:input()
  if Input.is_key_pressed(Input.SELECT) then
    self.speed = 1.0
  elseif Input.is_key_pressed(Input.START) then
    self.running = not self.running
  elseif Input.is_key_pressed(Input.UP) then
    self.speed = self.speed + 8.0
  elseif Input.is_key_pressed(Input.DOWN) then
    self.speed = self.speed - 8.0
  elseif Input.is_key_pressed(Input.LEFT) then
    self.angle = self.angle + math.pi * 0.01
  elseif Input.is_key_pressed(Input.RIGHT) then
    self.angle = self.angle - math.pi * 0.01
  end
end

function Game:update(delta_time)
  if not self.running then
    return
  end

  local cos, sin = math.cos(self.angle), math.sin(self.angle)

  self.x = self.x + (cos * -self.speed * delta_time)
  self.y = self.y + (sin * -self.speed * delta_time)
  self.surface:offset(-self.x, -self.y)

  local a, b = cos, sin
  local c, d = -sin, cos
  self.surface:matrix(a, b, c, d, Canvas.width() * 0.5, Canvas.height() * 0.5)
end

function Game:render(_)
  Canvas.clear()

  self.surface:xform()

  local cx, cy = Canvas.width() * 0.5, Canvas.height() * 0.5
  Canvas.line(cx, cy, cx + math.cos(self.angle) * 10, cy + math.sin(self.angle) * 10, 15)

  self.font:write(string.format("FPS: %d", System.fps()), 0, 0, "left")
end

return Game