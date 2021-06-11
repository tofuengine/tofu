--[[
MIT License

Copyright (c) 2019-2021 Marco Lizza

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

local Class = require("tofu.core").Class
local System = require("tofu.core").System
local Input = require("tofu.events").Input
local Canvas = require("tofu.graphics").Canvas
local Display = require("tofu.graphics").Display
local Palette = require("tofu.graphics").Palette
local Font = require("tofu.graphics").Font
local Speakers = require("tofu.sound").Speakers
local Pool = require("tofu.timers").Pool

local Main = require("main")

local Tofu = Class.define() -- To be precise, the class name is irrelevant since it's locally used.

function Tofu:__ctor()
  self.states = {
    ["normal"] = {
      enter = function(me)
          me.main = Main.new()
        end,
      leave = function(me)
          Pool.default():clear()
          me.main = nil
        end,
      input = function(me)
          me.main:input()
        end,
      update = function(me, delta_time)
          Pool.default():update(delta_time)
          me.main:update(delta_time)
        end,
      render = function(me, ratio)
          me.main:render(ratio)
        end
    },
    ["error"] = {
      enter = function(me)
          -- TODO: rename "Display" to "Video" e "Speakers" to "Audio"
          -- TODO: better failure screen with text.
          Display.reset()
          Display.palette(Palette.new({ { 0, 0, 0 }, { 255, 0, 0 } })) -- Red on black.
          local canvas = Canvas.default()
          local width, _ = canvas:size()
          canvas:pop(0) -- Discard all saved states, if any.
          canvas:reset() -- Reset default canvas from the game state.

          Speakers.halt() -- Stop all sounds sources.

          me.font = Font.default(0, 1)
          me.lines = {
              { text = "Software Failure." },
              { text = "Guru Meditation" }
            }

          local margin = 4 -- Pre-calculate lines position and rectangle area.
          local y = margin
          for _, line in ipairs(me.lines) do
            local lw, lh = me.font:size(line.text)
            line.x = (width - lw) * 0.5
            line.y = y
            y = y + lh
          end
          me.width = width
          me.height = y + margin
        end,
      leave = function(me)
          me.font = nil
        end,
      input = function(_)
          if Input.is_pressed("start") then
            System.quit()
          end
        end,
      update = function(_, _)
        end,
      render = function(me, _)
          local on = (math.floor(System.time()) % 2) == 0
          local canvas = Canvas.default()
          canvas:clear()
          canvas:rectangle("line", 0, 0, me.width, me.height, on and 1 or 0)
          for _, line in ipairs(me.lines) do
            me.font:write(canvas, line.x, line.y, line.text)
          end
        end
    }
  }
  self.queue = {}
  self:switch_to("normal")
end

function Tofu:input()
  self:switch_if_needed() -- TODO: add separate method for this?

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

function Tofu:switch_if_needed()
  if not next(self.queue) then
    return
  end

  local id = table.remove(self.queue)
  local entering = self.states[id]

  local exiting = self.state
  if exiting then
    self:call(exiting.leave, exiting)
  end
  self:call(entering.enter, entering)
  self.state = entering
end

function Tofu:switch_to(id)
  table.insert(self.queue, id)
end

function Tofu:call(func, ...)
  if next(self.queue) then
    return
  end
  local success, message = xpcall(func, debug.traceback, ...)
  if not success then
    System.error(message)
    self:switch_to("error")
  end
end

return Tofu.new()