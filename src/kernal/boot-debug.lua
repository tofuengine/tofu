--[[
                ___________________  _______________ ___
                \__    ___/\_____  \ \_   _____/    |   \
                  |    |    /   |   \ |    __) |    |   /
                  |    |   /    |    \|     \  |    |  /
                  |____|   \_______  /\___  /  |______/
                                   \/     \/
        ___________ _______    ________.___ _______  ___________
        \_   _____/ \      \  /  _____/|   |\      \ \_   _____/
         |    __)_  /   |   \/   \  ___|   |/   |   \ |    __)_
         |        \/    |    \    \_\  \   /    |    \|        \
        /_______  /\____|__  /\______  /___\____|__  /_______  /
                \/         \/        \/            \/        \

MIT License

Copyright (c) 2019-2024 Marco Lizza

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

local Class <const> = require("tofu.core.class")
local Log <const> = require("tofu.core.log")
local System <const> = require("tofu.core.system")
local Canvas <const> = require("tofu.graphics.canvas")
local Display <const> = require("tofu.graphics.display")
local Palette <const> = require("tofu.graphics.palette")
local Font <const> = require("tofu.graphics.font")
local Speakers <const> = require("tofu.sound.speakers")

local INITIAL_STATE <const> = "splash"

local Boot <const> = Class.define() -- To be precise, the class name is irrelevant since it's locally used.

function Boot:__ctor()
  self.states = {
    ["splash"] = { -- TODO: implement "splash" state that emulates Amiga's boot.
      enter = function(_)
          self:switch("running")
        end,
      leave = function(_)
        end,
      init = function(_)
        end,
      deinit = function(_)
        end,
      update = function(_, _)
        end,
      render = function(_, _)
        end
    },
    ["running"] = {
      enter = function(me)
          local Main <const> = require("main") -- Lazy require, to trap and display errors in the constructor!
          me.main = Main.new()
        end,
      leave = function(me)
        Speakers.halt() -- Stop all sounds sources.

        Display.reset()

        local canvas <const> = Canvas.default()
        canvas:pop() -- Discard all saved states, if any.
        canvas:reset() -- Reset default canvas from the game state.

        me.main = nil
        end,
      init = function(me)
          me.main:init()
        end,
      deinit = function(me)
          me.main:deinit()
        end,
      update = function(me, delta_time)
          me.main:update(delta_time)
        end,
      render = function(me, ratio)
          me.main:render(ratio)
        end
    },
    ["failure"] = {
      enter = function(me, message)
          me.message = message
        end,
      leave = function(me)
          me.message = nil
        end,
      init = function(me)
          -- TODO: rename "Display" to "Video" and "Speakers" to "Audio"?
          Display.palette(Palette.new({ { 0, 0, 0 }, { 255, 0, 0 } })) -- Red on black.
          local canvas <const> = Canvas.default()
          local width <const>, _ <const> = canvas:image():size()

          local title <const> = {
              "Software Failure.",
              "Guru Meditation"
            }
          local errors <const> = {}
          for str in string.gmatch(me.message, "([^\n]+)") do -- Split the error-message into separate lines.
            table.insert(errors, str)
          end

          me.font = Font.default(0, 1)
          me.lines = {}
          local margin <const> = 4 -- Pre-calculate lines position and rectangle area.
          local span <const> = width - 2 * margin
          local y = margin
          for _, text in ipairs(title) do -- Title lines are centered.
            local lw <const>, lh <const> = me.font:size(text)
            table.insert(me.lines, { text = text, x = (width - lw) * 0.5, y = y })
            y = y + lh
          end
          me.width = width
          y = y + margin
          me.height = y -- The rectangle ends here, message follows.
          y = y + margin
          for _, line in ipairs(errors) do -- Error lines are left-justified and auto-wrapped.
            local texts <const> = me.font:wrap(line, span)
            for _, text in ipairs(texts) do
              local _ <const>, th <const> = me.font:size(text)
              table.insert(me.lines, { text = text, x = margin, y = y })
              y = y + th
            end
          end
        end,
      deinit = function(me)
          me.font = nil
        end,
      update = function(_, _)
        end,
      render = function(me, _)
          local on <const> = (math.floor(System.time()) % 2) == 0
          local canvas <const> = Canvas.default()
          canvas:image():clear(0)
          canvas:rectangle("line", 0, 0, me.width, me.height, on and 1 or 0)
          for _, line in ipairs(me.lines) do
            canvas:write(line.x, line.y, me.font, line.text)
          end
        end
    }
  }
end

function Boot:init()
  self:switch(INITIAL_STATE)
end

function Boot:deinit()
  -- On close we switch to the `nil` state, which will cause the current one to be exited properly
  self:switch(nil)
end

function Boot:update(delta_time)
  local me <const> = self.state
  self:call(me.update, me, delta_time)
end

function Boot:render(ratio)
  local me <const> = self.state
  self:call(me.render, me, ratio)
end

function Boot:switch(id, ...)
  local exiting <const> = self.state
  if exiting then
    self:call(exiting.deinit, exiting)
    self:call(exiting.leave, exiting)
  end

  if not id then
    self.state = nil
    return
  end
  self.state = self.states[id]

  local entering <const> = self.state
  self:call(entering.enter, entering, ...)
  self:call(entering.init, entering)
end

function Boot:call(func, ...)
  local success <const>, message <const> = xpcall(func, debug.traceback, ...)
  if not success then
    Log.error(message) -- Dump to log...
    self:switch("failure", message) -- ... and pass to the error-state for visualization.
  end
end

return Boot.new()
