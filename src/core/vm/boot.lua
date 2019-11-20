local System = require("tofu.core").System
local Input = require("tofu.events").Input
local Canvas = require("tofu.graphics").Canvas
local Font = require("tofu.graphics").Font
local Class = require("tofu.util").Class

local Tofu = Class.define() -- To be precise, the class name is irrelevant since it's locally used.

function Tofu:setup()
  self.states = {
    ["normal"] = {
      input = function() self.main:input() end,
      update = function(delta_time) self.main:update(delta_time) end,
      render = function(ratio) self.main:render(ratio) end
    },
    ["error"] = {
      input = function()
          if Input.is_key_pressed(Input.RESET) then
            System.quit()
          end
        end,
      update = function(_) end,
      render = function(_)
        local w = Canvas.width()
        local fh = self.state.font:height()
        local on = (System.time() % 2) == 0
          Canvas.clear()
          Canvas.rectangle("line", 0, 0, w, fh * 2 + 8, on and 1 or 0)
          self.state.font:write("Software Failure.", w * 0.5, 0 + 4, "center")
          self.state.font:write("Guru Meditation", w * 0.5, fh + 4, "center")
        end
    }
  }
  self.state = self.states["normal"]
  return require("configuration")
end

function Tofu:init()
  local Main = require("main") -- Lazily require, to permit a `Tofu:setup()` call prior main script loading.
  self.main = Main.new()
end

function Tofu:input()
  self:try_catch(self.state.input)
end

function Tofu:update(delta_time)
  self:try_catch(self.state.update, delta_time)
end

function Tofu:render(ratio)
  self:try_catch(self.state.render, ratio)
end

function Tofu:try_catch(func, ...)
  local success, message = pcall(func, ...)
  if not success then
    if self.state == self.states["error"] then -- Failure while in error state bails out!
      error("internal error\n" .. message)
    end

    System.error(message)
    Canvas.palette({ "FF000000", "FFFF0000" })
    self.state = self.states["error"]
    self.state.font = Font.default(0, 1)
  end
end

return Tofu.new()
