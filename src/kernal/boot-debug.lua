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

Copyright (c) 2019-2026 Marco Lizza

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
local Mixer <const> = require("tofu.sound.mixer")

local INITIAL_STATE <const> = "splash"

local CANVAS <const> = Canvas.default()
local STATE <const> = CANVAS:state()

local Boot <const> = Class.define() -- To be precise, the class name is irrelevant since it's locally used.

function Boot:__ctor()
  self.states = {
    ["splash"] = {
      enter = function(me)
          local Splash <const> = require("splash")
          me.splash = Splash.new()
        end,
      leave = function(me)
          me.splash = nil
        end,
      init = function(me)
          me.splash:init()
        end,
      deinit = function(me)
          me.splash:deinit()
        end,
      update = function(me, delta_time)
          me.splash:update(delta_time)
          if me.splash.is_done() then -- Arbitrary time to switch to the "running" state.
            self:switch("running")
          end
        end,
      render = function(me, ratio)
          me.splash:render(ratio)
        end
    },
    ["running"] = {
      enter = function(me)
          if System.profile() then
            me.profile = require("profile")
            me.profile.start()
          end

          local Main <const> = require("main") -- Lazy require, to trap and display errors in the constructor!
          me.main = Main.new()
        end,
      leave = function(me)
          if me.profile then
            me.profile.stop()
            print(me.profile.report(32))
          end

          me.main = nil
        end,
      init = function(me)
          if not me.main then -- Sanity check, in case of an error in the `enter()` method.
            return
          end
          me.main:init()
        end,
      deinit = function(me)
          if not me.main then -- Ditto.
            return
          end
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
          local Panic <const> = require("panic")
          me.panic = Panic.new()
          me.panic:set_message(message)
        end,
      leave = function(me)
          me.panic = nil
        end,
      init = function(me)
          me.panic:init()
        end,
      deinit = function(me)
          me.panic:deinit()
        end,
      update = function(me, delta_time)
          me.panic:update(delta_time)
        end,
      render = function(me, ratio)
          me.panic:render(ratio)
        end
    }
  }
end

function Boot:init()
  self:switch(INITIAL_STATE)
end

function Boot:deinit()
  -- On close we switch to the `nil` state, which will cause the current one to be exited properly.
  self:switch(nil)
end

function Boot:update(delta_time)
  local me <const> = self.state
  local success <const>, message <const> = self:call(me.update, me, delta_time)
  if not success then
    self:handle_failure(message)
  end
end

function Boot:render(ratio)
  local me <const> = self.state
  local success <const>, message <const> = self:call(me.render, me, ratio)
  if not success then
    self:handle_failure(message)
  end
end

function Boot:reinit_system()
  Mixer.stop_all() -- Stop all sounds sources.

  Display.reset()

  STATE:pop() -- Discard all saved states, if any.
  STATE:reset() -- Reset default canvas from the game state.
end

function Boot:transition(id, ...)
  local exiting <const> = self.state

  -- Achtung #1: we set `self.state` to `nil` before calling the `leave()` and `deinit()`
  --             methods, so that they will be called at most once, even in case they fail
  --             and the `transition()` method is called again from the error handler.
  self.state = nil

  if exiting then
    local success, message = self:call(exiting.deinit, exiting)
    if not success then
      return false, message
    end
    success, message = self:call(exiting.leave, exiting)
    if not success then
      return false, message
    end
  end

  local success, message = self:call(self.reinit_system, self) -- Set to neutral, as when booted.
  if not success then
    return false, message
  end

  if not id then -- No state to enter, we are done with the transition.
    return true
  end

  local entering <const> = self.states[id]
  if not entering then
    return false, string.format("unknown boot state `%s`", id)
  end

  success, message = self:call(entering.enter, entering, ...)
  if not success then
    return false, message
  end
  success, message = self:call(entering.init, entering)
  if not success then
    return false, message
  end

  -- Achtung #2: we are storing the new state only after the `enter()` and `init()`
  --             methods have been called successfully. That way the state is
  --             guaranteed to be valid. Note that as a consequence we can't call
  --             `switch()` from within the `enter()` and `init()` methods, since
  --             the `self.state` is still `nil` at that point. Also a partially
  --             entered/initialized state won't be cleared and will be left
  --             dangling in memory (on purpose).
  self.state = entering

  return true
end

function Boot:handle_failure(message)
  Log.error(message)

  local success <const>, failure_message <const> = self:transition("failure", message)
  if not success then
    Log.error(failure_message)
    error(failure_message, 0)
  end
end

function Boot:switch(id, ...)
  local success <const>, message <const> = self:transition(id, ...)
  if not success then
    self:handle_failure(message)
  end
end

function Boot:call(func, ...)
  return xpcall(func, debug.traceback, ...)
end

return Boot.new()
