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

local Class = require("tofu.core.class")
local Iterators = require("tofu.util.iterators")

local MODES = {
  ["forward"] = Iterators.forward,
  ["reverse"] = Iterators.reverse,
  ["circular"] = Iterators.circular,
  ["bounce"] = Iterators.bounce,
}

local function get_iterator(mode, indexes)
  if not mode then
    return function()
      return indexes[1]
    end
  end
  return MODES[mode](indexes)
end

local Animation = Class.define()

local function build_frames(indexes, duration, flip_x, flip_y)
  local frames = {}
  for _, index in ipairs(indexes) do
    table.insert(frames, {
        cell_id = index,
        duration = duration,
        scale_x = flip_x and -1 or 1,
        scale_y = flip_y and -1 or 1
      })
  end
  return frames
end

function Animation:__ctor(bank, indexes, duration, mode, flip_x, flip_y)
  self.bank = bank
  self.frames = build_frames(indexes, duration, flip_x, flip_y)
  self.mode = mode

  self.active = true
  self:rewind()
end

function Animation:pause()
  self.active = false
end

function Animation:resume()
  self.active = true
end

function Animation:rewind()
  self.next = get_iterator(self.mode, self.frames)
  self.time = 0
  self.frame = self.next()
end

function Animation:update(delta_time)
  if not self.active then
    return
  end

  if self.frame.duration == 0 then
    return
  end

  self.time = self.time + delta_time

  while self.time >= self.frame.duration do
    self.time = self.time - self.frame.duration
    local frame = self.next()
    if not frame then
      self.active = false
      break
    end
    self.frame = frame
  end
end

function Animation:render(canvas, x, y)
  local frame = self.frame
  self.bank:blit(canvas, x, y, frame.cell_id, frame.scale_x, frame.scale_y)
end

return Animation