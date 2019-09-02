--local Canvas = require("tofu.graphics.Canvas")
--local Class = require("tofu.util.Class")
--local Timer = require("tofu.util.Timer")

local Canvas = require("tofu.graphics").Canvas
local Class = require("tofu.util").Class
local Timer = require("tofu.util").Timer

local Game = Class.define()

function Game:__ctor()
  Canvas.palette("pico-8")

  self.timerA = Timer.new(0.5, 50, function()
      --local o = Object.new()
      self.x = math.random() * Canvas.width()
    end)
  self.timerB = Timer.new(0.25, -1, function()
      self.y = math.random() * Canvas.height()
    end)
  self.timerC = Timer.new(15, 0, function()
      self.timerA:cancel()
      self.timerB = nil
    end)

    self.x = math.random() * Canvas.width()
    self.y = math.random() * Canvas.height()
end

function Game:input()
end

function Game:update(delta_time)
end

function Game:render(ratio)
  --local x = X.new()
  Canvas.circle("fill", self.x, self.y, 2, 1)
end

return Game
