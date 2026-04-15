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
local Speakers <const> = require("tofu.sound.speakers")

local INITIAL_STATE <const> = "splash"

local CANVAS <const> = Canvas.default()

local Boot <const> = Class.define() -- To be precise, the class name is irrelevant since it's locally used.

function Boot:__ctor()
  self.states = {
    ["splash"] = {
      enter = function(me)
          local Splash <const> = require("splash")
          me.splash = Splash.new()
        end,
      leave = function(_)
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
      render = function(me, canvas, _)
          me.splash:render(canvas)
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
      render = function(me, canvas, ratio)
          me.main:render(canvas, ratio)
        end
    },
    ["failure"] = {
      enter = function(me, message)
          local Panic <const> = require("panic")
          me.panic = Panic.new(message)
        end,
      leave = function(_)
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
      render = function(me, canvas, ratio)
          me.panic:render(canvas, ratio)
        end
    }
  }
end

function Boot:init()
  self:switch(INITIAL_STATE)
end

function Boot:deinit()
  -- On close we switch to the `nil` state, which will cause the current one to be exited properly
  --
  -- Note: the `xpcall()` code doesn't check if the current state is non `nil` (such as when
  -- we reach this piece of code). This isn't an issue as after the `Boot:deinit()` method is called
  -- the application is shut down and not other calls are made.
  self:switch(nil)
end

function Boot:update(delta_time)
  local me <const> = self.state
  self:call(me.update, me, delta_time)
end

function Boot:render(ratio)
  local me <const> = self.state
  self:call(me.render, me, CANVAS, ratio)
end

function Boot:reinit_system()
  Speakers.halt() -- Stop all sounds sources.

  Display.reset()

  CANVAS:pop() -- Discard all saved states, if any.
  CANVAS:reset() -- Reset default canvas from the game state.
end

-- Achtung! Don´t *ever* call `switch()` from within `enter()` or `leave()`
-- ======== methods! It is suggested also to avoid it from `init()` and
-- ======== `deinit()`, unless you really know what you are doing.
function Boot:switch(id, ...)
  local exiting <const> = self.state
  if exiting then
    self:call(exiting.deinit, exiting)
    self:call(exiting.leave, exiting)
  end

  self:reinit_system() -- Ensure that everything is neutral, as when booted.

  if not id then
    self.state = nil
    return
  end
  self.state = self.states[id] -- Store the new state so that `switch()` can work in `enter()` and `init()`.

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
