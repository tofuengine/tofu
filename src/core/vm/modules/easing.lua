--[[
MIT License

Copyright (c) 2019-2020 Marco Lizza

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

local Easing = {}

-- Inspired by the following:
--
-- https://github.com/sole/tween.js/blob/master/src/Tween.js
-- https://github.com/kikito/tween.lua/blob/master/tween.lua
-- https://gist.github.com/Fonserbc/3d31a25e87fdaa541ddf
-- https://github.com/mobius3/tweeny/blob/master/include/easing.h
-- Returns a function object bound to a specific easing function, duration and range of values.
-- The function object applies a single tweening evaluation, by normalizing the `time` argument
-- over the `duration`.
function Easing.tweener(...)
  local args = {...}
  if #args == 1 then
    local fn = Easing[args[1]]
    return fn -- calling the specific easing function directly.
  elseif #args == 2 then
    local fn = Easing[args[1]]
    local duration = args[2]
    return function(time)
        return fn(time / duration) -- Normalize the argument w/ regard to duration.
      end
  elseif #args == 4 then
    local fn = Easing[args[1]]
    local duration = args[2]
    local from = args[3]
    local to = args[4]
    return function(time)
        local r = fn(time / duration)
        return (1 - r) * from + r * to -- Precise method, which guarantees correct result `r = 1`.
      end
  end
end

return Easing
