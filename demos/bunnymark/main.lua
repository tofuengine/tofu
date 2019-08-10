local Class = require("tofu.util.class")

local Game = require("game")

local Main = Class.define()

function Main:__ctor()
  self.game = Game.new()
end

function Main:init()
  self.game:init()
end

function Main:input()
  self.game:input()
end

function Main:update(delta_time)
  self.game:update(delta_time)
end

function Main:render(ratio)
  self.game:render(ratio)
end

return Main
