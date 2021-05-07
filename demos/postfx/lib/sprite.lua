local Class = require("tofu.core").Class
local Canvas = require("tofu.graphics").Canvas

local Sprite = Class.define()

--local MIN_FREQUENCY = 0.25
--local MAX_FREQUENCY = 2.50
--local MAX_ANGLE = math.pi * 2
local PADDING = 16
local STEP = (2.0 * math.pi) / 32.0

function Sprite:__ctor(bank, index)
  self.bank = bank

  self.angle = STEP * index -- (2.0 * Num.pi) random.float() * MAX_ANGLE
  self.frequency_x = 0.75 --(random.float() * (MAX_FREQUENCY - MIN_FREQUENCY)) + MIN_FREQUENCY
  self.frequency_y = 0.50 --(random.float() * (MAX_FREQUENCY - MIN_FREQUENCY)) + MIN_FREQUENCY
  self.frequency_s = 3.00 --(random.float() * (MAX_FREQUENCY - MIN_FREQUENCY)) + MIN_FREQUENCY

  self.id = math.random(0, 11)

  local canvas = Canvas.default()
  local width, height = canvas:size()

  self.CENTER_X = width / 2
  self.CENTER_Y = (height - 64) / 2
  self.X_AMPLITUDE = (width / 2) - PADDING
  self.Y_AMPLITUDE = ((height - 64) / 2) - PADDING
end

function Sprite:update(delta_time)
  self.angle = self.angle + delta_time
end

function Sprite:render()
  local x = self.CENTER_X + math.cos(self.angle * self.frequency_x) * self.X_AMPLITUDE
  local y = self.CENTER_Y + math.sin(self.angle * self.frequency_y) * self.Y_AMPLITUDE

  local s = ((math.sin(self.angle * self.frequency_s) + 1.0) * 0.5) * 1.5 + 0.5
  self.bank:blit(self.id, x, y, s, s)
end

return Sprite