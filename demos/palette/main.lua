local Class = require("tofu.util").Class

local Main = Class.define()

function Main:setup()
  return {
      ["title"] = "Palette",
      ["width"] = 128,
      ["height"] = 128,
      ["scale"] = 2,
      ["fullscreen"] = false,
      ["fps-cap"] = 60,
      ["exit-key-enabled"] = true,
      ["debug"] = true
  }
end

function Main:init()
  local Game = require("game") -- Lazily require.
  self.game = Game.new()
end

function Main:input()
  --local x = M.new()
  self.game:input()
end

function Main:update(delta_time)
  self.game:update(delta_time)
end

function Main:render(ratio)
  self.game:render(ratio)
end

return Main
