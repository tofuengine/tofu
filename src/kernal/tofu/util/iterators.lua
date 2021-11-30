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

local function forward(t, looped)
  local n = #t
  local i = 0
  return function()
      i = i + 1
      if i > n then
        if looped and looped(t) then
          return nil
        end
        i = 0
      end
      return t[i]
    end
end

local function reverse(t, looped)
  local n = #t
  local i = n + 1
  return function()
      i = i - 1
      if i < 1 then
        if looped and looped(t) then
          return nil
        end
        i = n + 1
      end
      return t[i]
    end
end

local function circular(t, looped)
  local n = #t
  local i = 0
  return function()
      i = i + 1
      if i > n then
        if looped and looped(t) then
          return nil
        end
        i = 1
      end
      return t[i]
    end
end

local function bounce(t, bounced)
  local n = #t
  local d = 1
  local i = 0
  return function()
      i = i + d
      if d > 0 and i >= n then
        if bounced and bounced(t) then
          return nil
        end
        i = n
        d = -1
      elseif d < 0 and i <= 1 then
        if bounced and bounced(t) then
          return nil
        end
        i = 1
        d = 1
      end
      return t[i]
    end
end

-- Safe iterators that exclude the `nil` entries and additional check.
local function ipairs(table, check)
  return function(a, i)
      while true do
          i = i + 1
          local v = a[i]
          if not v then
            return nil, nil
          end
          if not check or check(v) then
            return i, v
          end
      end
    end, table, 0
end

local function reverse_ipairs(table, check)
  return function(a, i)
      while true do
        i = i - 1
        local v = a[i]
        if not v then
          return nil, nil
        end
        if not check or check(v) then
          return i, v
        end
      end
    end, table, #table + 1
end

local function pairs(table, check)
  return function(t, k)
      while true do
          local v = next(t, k)
          if not v then
            return nil, nil
          end
          if not check or check(v) then
            return k, v
          end
      end
    end, table, nil
end

return {
  forward = forward,
  reverse = reverse,
  circular = circular,
  bounce = bounce,
  ipairs = ipairs,
  reverse_ipairs = reverse_ipairs,
  pairs = pairs
}
