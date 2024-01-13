--[[
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

local Timer = Class.define()

function Timer:__ctor(period, repeats, callback, rate)
  self.period = period
  self.repeats = repeats
  self.callback = callback
  self.rate = rate or 1.0
--  self.age = 0.0
--  self.loops = repeats
--  self.cancelled = false

  self:reset()
end

function Timer:rate(rate)
  self.rate = rate or 1.0
end

function Timer:reset()
  self.age = 0.0
  self.loops = self.repeats
  self.cancelled = false
  self.callback("reset")
end

function Timer:cancel()
  self.cancelled = true
  self.callback("cancelled")
end

return Timer
