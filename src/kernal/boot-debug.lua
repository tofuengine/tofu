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

local Class = require("tofu.core.class")
local Log = require("tofu.core.log")
local System = require("tofu.core.system")
local Canvas = require("tofu.graphics.canvas")
local Display = require("tofu.graphics.display")
local Palette = require("tofu.graphics.palette")
local Font = require("tofu.graphics.font")
local Speakers = require("tofu.sound.speakers")

local Boot = Class.define() -- To be precise, the class name is irrelevant since it's locally used.

function Boot:__ctor()
  self.states = {
    -- TODO: add an "splash" state that emulates Amiga's boot.
    ["normal"] = {
      enter = function(me)
          local Main = require("main") -- Lazy require, to trap and display errors in the constructor!
          me.main = Main.new()
        end,
      leave = function(me)
          Speakers.halt() -- Stop all sounds sources.
          Display.reset()
          local canvas = Canvas.default()
          canvas:pop(0) -- Discard all saved states, if any.
          canvas:reset() -- Reset default canvas from the game state.
          me.main = nil
        end,
      process = function(me)
          me.main:process()
        end,
      update = function(me, delta_time)
          me.main:update(delta_time)
        end,
      render = function(me, ratio)
          me.main:render(ratio)
        end
    },
    ["error"] = {
      enter = function(me, message)
          -- TODO: rename "Display" to "Video" e "Speakers" to "Audio"
          Display.palette(Palette.new({ { 0, 0, 0 }, { 255, 0, 0 } })) -- Red on black.
          local canvas = Canvas.default()
          local width, _ = canvas:image():size()

          local title = {
              "Software Failure.",
              "Guru Meditation"
            }
          local errors = {}
          for str in string.gmatch(message, "([^\n]+)") do -- Split the error-message into separate lines.
            table.insert(errors, str)
          end

          me.font = Font.default(0, 1)
          me.lines = {}
          local margin <const> = 4 -- Pre-calculate lines position and rectangle area.
          local span <const> = width - 2 * margin
          local y = margin
          for _, text in ipairs(title) do -- Title lines are centered.
            local lw, lh = me.font:size(text)
            table.insert(me.lines, { text = text, x = (width - lw) * 0.5, y = y })
            y = y + lh
          end
          me.width = width
          y = y + margin
          me.height = y -- The rectangle ends here, message follows.
          y = y + margin
          for _, line in ipairs(errors) do -- Error lines are left-justified and auto-wrapped.
            local texts = me.font:wrap(line, span)
            for _, text in ipairs(texts) do
              local _, th = me.font:size(text)
              table.insert(me.lines, { text = text, x = margin, y = y })
              y = y + th
            end
          end
        end,
      leave = function(me)
          me.font = nil
        end,
      process = function(_, _)
        end,
      update = function(_, _)
        end,
      render = function(me, _)
          local on = (math.floor(System.time()) % 2) == 0
          local canvas = Canvas.default()
          canvas:image():clear(0)
          canvas:rectangle("line", 0, 0, me.width, me.height, on and 1 or 0)
          for _, line in ipairs(me.lines) do
            canvas:write(line.x, line.y, me.font, line.text)
          end
        end
    }
  }
  self.queue = {}
  self:switch_to("normal")
end

function Boot:process()
  self:switch_if_needed() -- TODO: `Tofu:process()` is the first method of the loop. Add separate method for this?

  local me = self.state
  self:call(me.process, me)
end

function Boot:update(delta_time)
  local me = self.state
  self:call(me.update, me, delta_time)
end

function Boot:render(ratio)
  local me = self.state
  self:call(me.render, me, ratio)
end

function Boot:switch_if_needed()
  if not next(self.queue) then
    return
  end

  local state = table.remove(self.queue)
  local entering = self.states[state.id]

  local exiting = self.state
  if exiting then
    self:call(exiting.leave, exiting)
  end
  self:call(entering.enter, entering, table.unpack(state.args))
  self.state = entering
end

function Boot:switch_to(id, ...)
  table.insert(self.queue, { id = id, args = { ... } })
end

function Boot:call(func, ...)
  if next(self.queue) then
    return
  end
  local success, message = xpcall(func, debug.traceback, ...)
  if not success then
    Log.error(message) -- Dump to log...
    self:switch_to("error", message) -- ... and pass to the error-state for visualization.
  end
end

return Boot.new()
