local Class = require("tofu.util").Class

local Main = Class.define()

function Main:setup()
  return {
      ["title"] = "BunnyMark",
      ["width"] = 512,
      ["height"] = 512,
      ["scale"] = 1,
      ["fullscreen"] = false,
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
