local Class = require("tofu.core").Class
local Iterators = require("tofu.util").Iterators

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

function Animation:blit(x, y)
  local frame = self.frame
  self.bank:blit(frame.cell_id, x, y, frame.scale_x, frame.scale_y)
end

return Animation