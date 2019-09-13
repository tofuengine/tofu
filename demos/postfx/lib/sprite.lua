local Canvas = require("tofu.graphics").Canvas
local Class = require("tofu.util").Class

local Sprite = Class.define()

local MIN_FREQUENCY = 0.25
local MAX_FREQUENCY = 2.50
local MAX_ANGLE = math.pi * 2
local CENTER_X = Canvas.width() / 2
local CENTER_Y = (Canvas.height() - 64) / 2
local PADDING = 16
local X_AMPLITUDE = (Canvas.width() / 2) - PADDING
local Y_AMPLITUDE = ((Canvas.height() - 64) / 2) - PADDING
local STEP = (2.0 * math.pi) / 32.0

function Sprite:__ctor(bank, index)
  self.bank = bank

  self.angle = STEP * index -- (2.0 * Num.pi) random.float() * MAX_ANGLE
  self.frequency_x = 0.75 --(random.float() * (MAX_FREQUENCY - MIN_FREQUENCY)) + MIN_FREQUENCY
  self.frequency_y = 0.50 --(random.float() * (MAX_FREQUENCY - MIN_FREQUENCY)) + MIN_FREQUENCY
  self.frequency_s = 3.00 --(random.float() * (MAX_FREQUENCY - MIN_FREQUENCY)) + MIN_FREQUENCY

  self.id = math.random(0, 11)
end

function Sprite:update(delta_time)
  self.angle = self.angle + delta_time
end

function Sprite:render()
  local x = CENTER_X + math.cos(self.angle * self.frequency_x) * X_AMPLITUDE
  local y = CENTER_Y + math.sin(self.angle * self.frequency_y) * Y_AMPLITUDE

  local s = ((math.sin(self.angle * self.frequency_s) + 1.0) / 2.0) * 1.5 + 0.5
  --local o = self.bank:cell_width() * s * 0.5
  --self.bank:blit(self.id, x - o, y - o, s, s)
  self.bank:blit(self.id, x, y, s, s, 0.0, 0.5, 0.5)
end

return Sprite