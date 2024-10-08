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
local Timer = require("tofu.timers.timer")

local Pool = Class.define()

function Pool:__ctor()
  self.timers = {}
end

function Pool:spawn(period, repeats, callback, rate)
  local timer = Timer.new(period, repeats, callback, rate)
  table.insert(self.timers, timer)
  return timer
end

function Pool:clear()
  self.timers = {}
end

function Pool:update(delta_time)
  local timers = self.timers -- Use local for faster access.

  for index = #timers, 1, -1 do -- Reverse iterate to remove timers when "dead".
    local timer = timers[index]
    if timer.cancelled then
      table.remove(timers, index)
    else
      timer.age = timer.age + timer.rate * delta_time
      while timer.age >= timer.period do
        timer.callback("fired")

        timer.age = timer.age - timer.period

        if timer.loops > 0 then
          timer.callback("looped")
          timer.loops = timer.loops - 1;
          if timer.loops == 0 then
              timer:cancel() -- Remove on the next iteration.
              break
          end
        end
      end
    end
  end
end

return Pool
