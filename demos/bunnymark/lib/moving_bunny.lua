local Class = require("tofu.core").Class
local Canvas = require("tofu.graphics").Canvas

local Bunny = Class.define()

local CELL_ID = 1
local MAX_SPEED = 500
local GRAVITY = 981
local X_DAMPENING = 0.95
local Y_DAMPENING = 0.85
local MIN_X, MIN_Y = 0, 0
local MAX_X, MAX_Y = Canvas.default():size()

function Bunny:__ctor(bank, batch)
  local cw, ch = bank:size(CELL_ID)

  self.min_x = MIN_X
  self.min_y = MIN_Y
  self.max_x = MAX_X - cw
  self.max_y = MAX_Y - ch

  self.batch = batch
  self.x = (self.max_x - self.min_x) / 2 -- Spawn in the top-center part of the screen.
  self.y = (self.max_y - self.min_y) / 8
  self.vx = (math.random() * MAX_SPEED) - (MAX_SPEED / 2.0)
  self.vy = (math.random() * MAX_SPEED) - (MAX_SPEED / 2.0)

  batch:add({ cell_id = CELL_ID, x = self.x, y = self.y })
end

function Bunny:update(delta_time)
  self.x = self.x + self.vx * delta_time
  self.y = self.y + self.vy * delta_time

  self.vy = self.vy + GRAVITY * delta_time

  if self.x > self.max_x then
    self.vx = self.vx * X_DAMPENING * -1.0
    self.x = self.max_x
  elseif self.x < self.min_x then
    self.vx = self.vx * X_DAMPENING * -1.0
    self.x = self.min_x
  end

  if self.y > self.max_y then
    self.vy = self.vy * Y_DAMPENING * -1.0
    self.y = self.max_y

    if math.abs(self.vy) <= 400.0 and math.random() <= 0.10 then  -- Jump, occasionally!
      self.vx = self.vx - ((math.random() * 100.0) - 50.0)
      self.vy = self.vy - ((math.random() * 500.0) + 250.0)
    end
  elseif self.y < self.min_y then
    self.vy = 0.0 -- Bump on the ceiling!
    self.y = self.min_y
  end

  self.batch:add({ cell_id = CELL_ID, x = self.x, y = self.y })
end

return Bunny