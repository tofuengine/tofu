--[[
  Copyright (c) 2019 Marco Lizza (marco.lizza@gmail.com)

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
]]--

local System = require("tofu.core").System
local Input = require("tofu.events").Input
local Canvas = require("tofu.graphics").Canvas
local Font = require("tofu.graphics").Font
local Class = require("tofu.util").Class

local Tofu = Class.define() -- To be precise, the class name is irrelevant since it's locally used.

function Tofu:setup()
  self.configuration = require("configuration")
  return self.configuration
end

function Tofu:init()
  self.states = {
    ["splash"] = {
      enter = function(state)
          state.duration = 2.5
          state.time = 0.0
        end,
      leave = function(_)
        end,
      input = function(_)
        end,
      update = function(state, delta_time)
          state.time = state.time + delta_time
          if state.time >= state.duration then
            self:switch_to("normal")
          end
        end,
      render = function(state, _)
          local y = math.floor((1.0 - state.time / state.duration) * 255.0)
          Canvas.clear()
          Canvas.rectangle("fill", 0, 0, Canvas.width(), Canvas.height(), y)
        end
    },
    ["normal"] = {
      enter = function(state)
          local Main = require("main") -- Lazily require, to permit a `Tofu:setup()` call prior main script loading.
          state.main = Main.new()
        end,
      leave = function(state)
          state.main = nil
        end,
      input = function(state)
          state.main:input()
        end,
      update = function(state, delta_time)
          state.main:update(delta_time)
        end,
      render = function(state, ratio)
          state.main:render(ratio)
        end
    },
    ["error"] = {
      enter = function(state)
          Canvas.palette({ 0xFF000000, 0xFFFF0000 })
          state.font = Font.default(0, 1)
        end,
      leave = function(state)
          state.font = nil
        end,
      input = function(_)
          if Input.is_key_pressed(Input.RESET) then
            System.quit()
          end
        end,
      update = function(_, _)
        end,
      render = function(state, _)
        local w = Canvas.width() -- TODO: could precalculate these values.
        local fh = state.font:height()
        local on = (System.time() % 2) == 0
          Canvas.clear()
          Canvas.rectangle("line", 0, 0, w, fh * 2 + 8, on and 1 or 0)
          state.font:write("Software Failure.", w * 0.5, 0 + 4, "center")
          state.font:write("Guru Meditation", w * 0.5, fh + 4, "center")
        end
    }
  }
  self:switch_to(self.configuration["splash-screen"] and "splash" or "normal")
end

function Tofu:deinit()
  local state = self.state
  self:call(state.leave, state)
end

function Tofu:input()
  local state = self.state
  self:call(state.input, state)
end

function Tofu:update(delta_time)
  local state = self.state
  self:call(state.update, state, delta_time)
end

function Tofu:render(ratio)
  local state = self.state
  self:call(state.render, state, ratio)
end

function Tofu:switch_to(id)
  local current = self.state
  local next = self.states[id]
  if current == next then -- Moving to the current state is an error!
    return false
  end
  if current then
    self:call(current.leave, current)
    self.state = nil
  end
  self:call(next.enter, next)
  self.state = next
  return true
end

function Tofu:call(func, ...)
  local success, message = pcall(func, ...)
  if not success then
    if not self:switch_to("error") then
      error("internal error\n" .. message)
    end
    System.error(message .. "\n" .. debug.traceback())
  end
end

return Tofu.new()
