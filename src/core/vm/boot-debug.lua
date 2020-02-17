--[[
MIT License

Copyright (c) 2019-2020 Marco Lizza

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
local Display = require("tofu.graphics").Display
local Font = require("tofu.graphics").Font
local Class = require("tofu.util").Class
local Timer = require("tofu.util").Timer

local Main = require("main")

local Tofu = Class.define() -- To be precise, the class name is irrelevant since it's locally used.

function Tofu:__ctor()
  self.states = {
    ["normal"] = {
      enter = function(me)
          me.main = Main.new()
        end,
      leave = function(me)
          Timer.pool:clear()
          me.main = nil
        end,
      process = function(me)
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
          Display.palette({ 0xFF000000, 0xFFFF0000 })
          me.canvas = Canvas.new()
          me.font = Font.default("5x8", 0, 1)
        end,
      leave = function(me)
          me.font = nil
        end,
      process = function(_)
          if Input.is_pressed("start") then
            System.quit()
          end
        end,
      update = function(_, _)
        end,
      render = function(me, _)
          local w, _ = me.canvas:size() -- TODO: could precalculate these values.
          local _, fh = me.font:size()
          local on = (System.time() % 2) == 0
          me.canvas:clear()
          me.canvas:rectangle("line", 0, 0, w, fh * 2 + 8, on and 1 or 0)
          me.font:write(me.font:align("Software Failure.", w * 0.5, 0 + 4, "center"))
          me.font:write(me.font:align("Guru Meditation", w * 0.5, fh + 4, "center"))
        end
    }
  }
  self.queue = {}
  self:switch_to("normal")
end

function Tofu:process()
  self:switch_if_needed()

  local me = self.state
  self:call(me.process, me)
end

function Tofu:update(delta_time)
  local me = self.state
  self:call(me.update, me, delta_time)
end

function Tofu:render(ratio)
  local me = self.state
  self:call(me.render, me, ratio)
end

function Tofu:switch_if_needed()
  if not next(self.queue) then
    return
  end

  local id = table.remove(self.queue)
  local next = self.states[id]

  local current = self.state
  if current then
    current:leave()
  end
  next:enter()
  self.state = next
end

function Tofu:switch_to(id)
  table.insert(self.queue, id)
end

function Tofu:call(func, ...)
  if next(self.queue) then
    return
  end
  local success, message = pcall(func, ...)
  if not success then
    System.error(message)
    self:switch_to("error")
  end
end

return Tofu.new()
