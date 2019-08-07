local events = require("tofu.events")
local graphics = require("tofu.graphics")
local io = require("tofu.io")

local Game = {}

Game.__index = Game

function Game.new()
  for k, v in pairs(graphics) do
    print(k)
  end
  for k, v in pairs(graphics.Font) do
    print(k)
  end

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