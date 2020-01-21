local Canvas = require("tofu.graphics").Canvas
local Class = require("tofu.util").Class

local Bunny = Class.define()

local MAX_SPEED = 500
local GRAVITY = 981
local DAMPENING = 0.9
local MIN_X = 0
local MAX_X = Canvas.default():width() - 26
local MIN_Y = 0
local MAX_Y = Canvas.default():height() - 37

function Bunny:__ctor(bank)
  self.bank = bank
  self.x = (MAX_X - MIN_X) / 2 -- Spawn in the top-center part of the screen.
  self.y = (MAX_Y - MIN_Y) / 8
  self.vx = (math.random() * MAX_SPEED) - (MAX_SPEED / 2.0)
  self.vy = (math.random() * MAX_SPEED) - (MAX_SPEED / 2.0)
end

function Bunny:update(delta_time)
  self.x = self.x + self.vx * delta_time
  self.y = self.y + self.vy * delta_time

  self.vy = self.vy + GRAVITY * delta_time

  if self.x > MAX_X then
    self.vx = self.vx * DAMPENING * -1.0
    self.x = MAX_X
  elseif self.x < MIN_X then
    self.vx = self.vx * DAMPENING * -1.0
    self.x = MIN_X
  end

  if self.y > MAX_Y then
    self.vy = self.vy * DAMPENING * -1.0
    self.y = MAX_Y

    if math.abs(self.vy) <= 400.0 and math.random() <= 0.10 then  -- Higher bounce occasionally.
      self.vy = self.vy - ((math.random() * 150.0) + 100.0)
    end
  elseif self.y < MIN_Y then
    self.vy = self.vy * DAMPENING * -1.0
    self.y = MIN_Y
  end
end

function Bunny:render()
  self.bank:blit(0, self.x, self.y)
end

return Bunny