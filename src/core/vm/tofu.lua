local Class = require("tofu.util").Class

local Tofu = Class.define() -- To be precise, the class name is irrelevant since it's locally used.

function Tofu:setup()
  return require("configuration")
end

function Tofu:init()
  local Main = require("main") -- Lazily require.
  self.main = Main.new()
end

function Tofu:input()
  self.main:input()
end

function Tofu:update(delta_time)
  self.main:update(delta_time)
end

function Tofu:render(ratio)
  self.main:render(ratio)
end

return Tofu.new()
