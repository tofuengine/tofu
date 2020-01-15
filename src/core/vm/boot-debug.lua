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
      { id = "32x64", width = 352 },
      { id = "16x32", width = 176 },
      { id = "12x24", width = 132 },
      { id = "8x16", width = 88 },
      { id = "5x8", width = 55 },
    }
  for _, font in ipairs(t) do
    if width >= font.width * ratio then
      return font.id
    end
  end
end

function Tofu:__ctor()
  self.states = {
    ["splash"] = {
      enter = function(me)
          Canvas.reset()
          me.font = Font.new(get_font_name(Canvas.width(), 1.5), 0, 255)
          me.duration = 5.0
          me.age = 0.0
        end,
      leave = function(me)
          me.font = nil
        end,
      process = function(_)
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
            me.font:write(me.font:align("made with", Canvas.width() * 0.5, y, "center"))
            me.font:write(me.font:align("TOFU ENGINE", Canvas.width() * 0.5, y + fh, "center"))
          Canvas.pop()
        end
    },
    ["running"] = {
      enter = function(me)
          Canvas.reset()
          local Main = require("main") -- Lazily require, to permit a `Tofu:setup()` call prior main script loading.
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
          Canvas.reset()
          Canvas.palette({ 0xFF000000, 0xFFFF0000 })
          me.font = Font.default(0, 1)
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
          local w = Canvas.width() -- TODO: could precalculate these values.
          local fh = me.font:height()
          local on = (System.time() % 2) == 0
          Canvas.clear()
          Canvas.rectangle("line", 0, 0, w, fh * 2 + 8, on and 1 or 0)
          me.font:write(me.font:align("Software Failure.", w * 0.5, 0 + 4, "center"))
          me.font:write(me.font:align("Guru Meditation", w * 0.5, fh + 4, "center"))
        end
    }
  }
  self.queue = {}
  self:switch_to("running") -- "splash")
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
