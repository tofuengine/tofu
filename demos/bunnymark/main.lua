local Game = require("game")

local Main = {}

Main.__index = Main

function Main.new()
  return setmetatable({
      game = Game.new()
    }, Main)
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
