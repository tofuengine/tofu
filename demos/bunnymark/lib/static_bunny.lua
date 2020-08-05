local Class = require("tofu.core").Class
local Canvas = require("tofu.graphics").Canvas

local Bunny = Class.define()

local CELL_ID = 0
local MIN_X, MIN_Y = 0, 0
local MAX_X, MAX_Y = Canvas.default():size()

function Bunny:__ctor(bank, batch)
  local cw, ch = bank:size(CELL_ID)

  local min_x = MIN_X + cw
  local min_y = MIN_Y + cw
  local max_x = MAX_X - cw * 2
  local max_y = MAX_Y - ch * 2

  self.batch = batch
  self.x = math.random() * (max_x - min_x) + cw
  self.y = math.random() * (max_y - min_y) + ch

  batch:add({ cell_id = CELL_ID, x = self.x, y = self.y })
end

function Bunny:update(_)
  self.batch:add({ cell_id = CELL_ID, x = self.x, y = self.y })
end

return Bunny