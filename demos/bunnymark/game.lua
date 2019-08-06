local events = require("events")

local Game = {}

Game.__index = Game

function Game.new()
  return setmetatable({}, Game)
end

function Game:input()
  print(events.Input)
  if events.Input.is_key_pressed(Input.START) then
  end
end

function Game:update(delta_time)
end

function Game:render(ratio)
end

return Game