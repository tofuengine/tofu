local Bank = require("tofu.graphics").Bank
local Canvas = require("tofu.graphics").Canvas
local Font = require("tofu.graphics").Font
local Input = require("tofu.events").Input
local Class = require("tofu.util").Class
local System = require("tofu.core").System

local Game = Class.define()

local AMOUNT = 16
local PALETTES = { "pico-8", "arne-16", "c64", "cga" }

function Game:__ctor()
  Canvas.palette("pico-8")

  self.bank = Bank.new("assets/sheet.png", 8, 8)
  self.font = Font.default(0, 15)
  self.time = 0
  self.x_size = Canvas.width() / AMOUNT
  self.y_size = Canvas.height() / AMOUNT
  self.palette = 1
end

function Game:input()
  if Input.is_key_pressed(Input.SELECT) then
    self.palette = (self.palette % #PALETTES) + 1
    Canvas.palette(PALETTES[self.palette])
  end
end

function Game:update(delta_time)
  self.time = self.time + delta_time
  local index = (math.floor(self.time * 0.2) % #PALETTES) + 1
  if self.palette ~= index then
    self.palette = index
    Canvas.palette(PALETTES[index])
  end
end

function Game:render(ratio)
  for i = 0, AMOUNT - 1 do
    local x = self.x_size * i
    for j = 0, AMOUNT - 1 do
      local index = (i + j) % 3
      local color = (i + j) % AMOUNT
      local y = Canvas.height() * (math.sin(self.time * 1.5 + i * 0.250 + j * 0.125) + 1) * 0.5
      Canvas.shift(1, color)
      self.bank:blit(index, x, y)
      end
  end

  self.font:write(string.format("FPS: %d", System.fps()), 0, 0, 1, "left")
end

return Game
