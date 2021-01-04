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

local Pool = {}

Pool.__index = Pool

local _default = nil

function Pool.default()
  if not _default then
    _default = Pool.new()
  end
  return _default
end

function Pool.new()
  return setmetatable({ timers = {} }, Pool)
end

function Pool:clear()
  self.timers = {}
end

function Pool:update(delta_time)
  local zombies = {}

  for index, timer in ipairs(self.timers) do
    if timer.cancelled then
      table.insert(zombies, index)
    else
      timer.age = timer.age + timer.rate * delta_time
      while timer.age >= timer.period do
        timer.callback()

        timer.age = timer.age - timer.period

        if timer.loops > 0 then
          timer.loops = timer.loops - 1;
          if timer.loops == 0 then
              timer:cancel()
              break
          end
        end
      end
    end
  end

  for _, index in ipairs(zombies) do
    table.remove(self.timers, index)
  end
end

local Timer = {}

Timer.__index = Timer

function Timer.new(period, repeats, callback, pool)
  local self = setmetatable({
      period = period,
      repeats = repeats,
      callback = callback,
      rate = 1.0,
      age = 0.0,
      loops = repeats,
      cancelled = false
    }, Timer)
  table.insert((pool or Pool.default()).timers, self) -- Access inner field to avoid exposing an API method.
  return self
end

function Timer:rate(rate)
  self.rate = rate or 1.0
end

function Timer:reset()
  self.age = 0.0
  self.loops = self.repeats
  self.cancelled = false
end

function Timer:cancel()
  self.cancelled = true
end

return {
    Pool = Pool,
    Timer = Timer
  }