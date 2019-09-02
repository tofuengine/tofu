local Grid = require("tofu.collections").Grid
local Canvas = require("tofu.graphics").Canvas
local Font = require("tofu.graphics").Font
local Input = require("tofu.events").Input
local Class = require("tofu.util").Class
local System = require("tofu.core").System

local STEPS = 64
local PALETTE = {
    "FF000000", "FF240000", "FF480000", "FF6D0000",
    "FF910000", "FFB60000", "FFDA0000", "FFFF0000",
    "FFFF3F00", "FFFF7F00", "FFFFBF00", "FFFFFF00",
    "FFFFFF3F", "FFFFFF7F", "FFFFFFBF", "FFFFFFFF"
  }

local Game = Class.define()

function Game:__ctor()
  Canvas.palette(PALETTE)

  self.font = Font.default()
  self.x_size = Canvas.width() / STEPS
  self.y_size = Canvas.height() / STEPS
  self.windy = false
  self.grid = Grid.new(STEPS, STEPS, 0)

  self:reset()
end

function Game:reset()
  self.grid:fill(0)
  self.grid:stride(0, STEPS - 1, #PALETTE - 1, STEPS)
end

function Game:input()
  if Input.is_key_pressed(Input.SELECT) then
    self.windy = not self.windy
  end
end

function Game:update(delta_time)
  for i = 0, STEPS - 2 do
    for j = 0, STEPS - 1 do
      local value = self.grid:peek(j, i + 1)
      if value > 0 then
        local delta = math.random(0, 2)
        value = value - delta
        if value < 0 then
            value = 0
        end
      end

      local x = j
      if self.windy then
        x = x + math.random(0, 3) - 1
        if x < 0 then
          x = 0
        end
        if x >= STEPS then
          x = STEPS - 1
        end
      end

      self.grid:poke(x, i, value)
    end
  end
end

function Game:render(ratio)
  for i = 0, STEPS - 1 do
    local y = i * self.y_size
    for j = 0, STEPS - 1 do
      local x = j * self.x_size
      local value = self.grid:peek(j, i)
      if value > 0 then
        Canvas.rectangle("fill", x, y, self.x_size, self.y_size, math.floor(value))
      end
    end
  end

  self.font:write(string.format("FPS: %d", System.fps()), 0, 0, 15, 1.0, "left")
end

return Game
