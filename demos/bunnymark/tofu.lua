local Game = require("game")

local Tofu = {}

Tofu.__index = Tofu

function Tofu.new()
  self.game = Game.new()
end

function Tofu:input()
  self.game:input()
end

function Tofu:update(delta_time)
  self.game:update(delta_time)
end

function Tofu:render(ratio)
  self.game:render(ratio)
end

return Tofu
