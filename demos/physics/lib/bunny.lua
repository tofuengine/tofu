local Class = require("tofu.core").Class
local Math = require("tofu.core").Math
local Canvas = require("tofu.graphics").Canvas
local Body = require("tofu.physics").Body

local Bunny = Class.define()

local CELL_ID = 0
local MIN_X, MIN_Y = 0, 0
local MAX_X, MAX_Y = Canvas.default():size()

function Bunny:__ctor(font, bank)
  local cw, ch = bank:size(CELL_ID)

  local min_x = MIN_X + cw
  local min_y = MIN_Y + cw
  local max_x = MAX_X - cw * 2
  local max_y = MAX_Y - ch * 2

  self.font = font

  self.bank = bank
  local x = math.random() * (max_x - min_x) + cw
  local y = math.random() * (max_y - min_y) + ch

  local body = Body.new()
  body:shape("box", cw, ch)
  body:elasticity(0.75)
  body:type("dynamic")
  body:mass(25)
  body:momentum(10)
  body:position(x, y)
  self.body = body
end

function Bunny:update(_)
end

function Bunny:render(canvas)
  local x, y = self.body:position()
  local _, ch = self.bank:size(CELL_ID)
  local angle = self.body:angle()
  self.bank:blit(canvas, x, y, CELL_ID, Math.angle_to_rotation(angle))
  self.font:write(canvas, x, y - ch * 0.5, string.format("%d, %d", x, y), "center", "middle")
end

return Bunny