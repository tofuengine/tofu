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
local Wave = require("tofu.generators").Wave

local Oscillator = Class.define()

-- See: https://blog.demofox.org/2012/05/19/diy-synthesizer-chapter-2-common-wave-forms/

function Oscillator:__ctor(...)
  self.wave = Wave.new(...)
  self.phase = 0
end

function Oscillator:advance(delta_phase)
  local phase = self.phase + delta_phase
  local period = self.wave:period()
  while phase >= period do -- Keep constrained in [0, period) in order not to loose precision.
    phase = phase - period
  end
  while phase < 0.0 do
    phase = phase + period
  end
  self.phase = phase
end

function Oscillator:value(offset)
  return self.wave(self.phase + (offset or 0.0))
end

return Oscillator