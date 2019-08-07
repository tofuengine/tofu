local events = require("events")
local graphics = require("graphics")

local Game = {}

Game.__index = Game

function Game.new()
  return setmetatable({
      font = graphics.Font.default()
    }, Game)
end

function Game:input()
  if events.Input.is_key_pressed(events.Input.SPACE) then
  end
end

function Game:update(delta_time)
end

function Game:render(ratio)
  self.font:write("HELLO", 0, 0, 65, 2, "left")
end

return Game