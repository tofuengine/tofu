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

  self.font = Font.default(0, 15)
  self.x_size = Canvas.width() / STEPS
  self.y_size = Canvas.height() / STEPS
  self.windy = false
  self.damping = 1.0
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
  elseif Input.is_key_pressed(Input.LEFT) then
    self.damping = self.damping - 0.1
  elseif Input.is_key_pressed(Input.RIGHT) then
    self.damping = self.damping + 0.1
  elseif Input.is_key_pressed(Input.START) then
    self:reset()
  end
end

function Game:update(_)
  local windy = self.windy
  self.grid:process(function(column, row, value)
      if row == 0 then -- Skip the 1st row entirely.
        return column, row, value
      end

      if value > 0 then
        local delta = math.random(0, 1) * self.damping
        value = value - delta
        if value < 0 then
          value = 0
        end
      end

      if windy then
        column = column + math.random(0, 3) - 1
        if column < 0 then
          column = 0
        elseif column > STEPS - 1 then
          column = STEPS - 1
        end
      end

      return column, row - 1, value -- Move up one row!
    end)
end

function Game:render(_)
  Canvas.clear()

  local w = self.x_size
  local h = self.y_size
  self.grid:scan(function(column, row, value)
      local x = column * w
      local y = row * h
      if value > 0 then
        Canvas.rectangle("fill", x, y, w, h, math.floor(value))
      end
    end)

    self.font:write(string.format("FPS: %d", System.fps()), 0, 0, "left")
    self.font:write(string.format("D: %.2f", self.damping), Canvas.width(), 0, "right")
end

return Game
