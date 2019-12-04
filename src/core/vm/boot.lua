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

local Math = require("tofu.core").Math
local System = require("tofu.core").System
local Input = require("tofu.events").Input
local Canvas = require("tofu.graphics").Canvas
local Font = require("tofu.graphics").Font
local Class = require("tofu.util").Class
local Timer = require("tofu.util").Timer

local Tofu = Class.define() -- To be precise, the class name is irrelevant since it's locally used.

local function get_font_name(width, ratio)
  local t = {
      ["32x64"] = 352,
      ["16x32"] = 176,
      ["12x24"] = 132,
      ["8x16"] = 88,
      ["5x8"] = 55,
    }
  for id, span in pairs(t) do
    if span * ratio >= width then
      return id
    end
  end
end

function Tofu:__ctor()
  self.states = {
    ["idle"] = {
      enter = function(_)
        end,
      leave = function(_)
        end,
      input = function(_)
        end,
      update = function(_, _)
          self:switch_to(self.configuration["splash-screen"] and "splash" or "normal")
          -- FIXME: don't call render after switch!!!
        end,
      render = function(_, _)
        end
    },
    ["splash"] = {
      enter = function(me)
          me.font = Font.new(get_font_name(Canvas.width(), 2.0), 0, 255)
          me.duration = self.configuration["splash-screen-duration"] or 10.0
          me.age = 0.0
        end,
      leave = function(me)
          me.font = nil
        end,
      input = function(_)
        end,
      update = function(me, delta_time)
          me.age = me.age + delta_time
          if me.age >= me.duration then
            self:switch_to("running")
          end
        end,
      render = function(me, _)
          local fh = me.font:height()
          local y = (Canvas.height() - fh * 2) * 0.5
--          local v = math.tointeger(((Math.triangle_wave(me.duration, me.age) + 1) * 0.5) * 255.0)
          local v = math.min(math.tointeger(((Math.triangle_wave(me.duration, me.age) + 1) * 255.0)), 255)
          Canvas.clear()
          Canvas.push()
            Canvas.shift({ [255] = v })
            me.font:write("made with", Canvas.width() * 0.5, y, "center")
            me.font:write("TOFU ENGINE", Canvas.width() * 0.5, y + fh, "center")
          Canvas.pop()
        end
    },
    ["running"] = {
      enter = function(me)
          local Main = require("main") -- Lazily require, to permit a `Tofu:setup()` call prior main script loading.
          me.main = Main.new()
        end,
      leave = function(me)
          Timer.pool:clear()
          me.main = nil
        end,
      input = function(me)
          me.main:input()
        end,
      update = function(me, delta_time)
          Timer.pool:update(delta_time)
          me.main:update(delta_time)
        end,
      render = function(me, ratio)
          me.main:render(ratio)
        end
    },
    ["error"] = {
      enter = function(me)
          Canvas.palette({ 0xFF000000, 0xFFFF0000 })
          me.font = Font.default(0, 1)
        end,
      leave = function(me)
          me.font = nil
        end,
      input = function(_)
          if Input.is_key_pressed(Input.RESET) then
            System.quit()
          end
        end,
      update = function(_, _)
        end,
      render = function(me, _)
          local w = Canvas.width() -- TODO: could precalculate these values.
          local fh = me.font:height()
          local on = (System.time() % 2) == 0
          Canvas.clear()
          Canvas.rectangle("line", 0, 0, w, fh * 2 + 8, on and 1 or 0)
          me.font:write("Software Failure.", w * 0.5, 0 + 4, "center")
          me.font:write("Guru Meditation", w * 0.5, fh + 4, "center")
        end
    }
  }
  self.state = self.states["idle"]
end

function Tofu:setup()
  self.configuration = require("configuration")
  return self.configuration
end

function Tofu:input()
  local me = self.state
  self:call(me.input, me)
end

function Tofu:update(delta_time)
  local me = self.state
  self:call(me.update, me, delta_time)
end

function Tofu:render(ratio)
  local me = self.state
  self:call(me.render, me, ratio)
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
