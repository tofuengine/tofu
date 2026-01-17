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

-- http://developer.classpath.org/doc/java/util/Collections-source.html

-- FIXME: adopt build pattern and remove copies?

local function erase_if(array, callback) -- callback(value, index, length, array)
  local erased = 0
  local length <const> = #array
  for index = length, 1, -1 do
    local value <const> = array[index]
    local erase <const>, stop <const> = callback(value, index, length, array)
    if erase then
      table.remove(array, index)
      erased = erased + 1
    end
    if stop then
      break
    end
  end
  return erased
end

local function displace(array, callback) -- shuffle (?), rotate, reverse
  local result <const> = {}
  local length <const> = #array
  for index = 1, length do
    local value <const> = array[index]
    result[callback(value, index, length, array)] = value
  end
  return result
end

local function generate(callback)
  local result <const> = {}
  local index = 1
  while true do
    local value <const> = callback(index)
    if value == nil then
      break
    end
    result[index] = value
    index = index + 1
  end
  return result
end

local function index_of(array, compare, from)
  local length <const> = #array
  for index = from or 1, length do
    local value <const> = array[index]
    if compare(value) then
      return index
    end
  end
  return nil
end

local function reversed(array)
  local result <const> = {}
  local length <const> = #array
  for index = 1, length do
    result[length - index + 1] = array[index]
  end
  return result
end

local function reverse(array)
  local length <const> = #array
  for index = 1, math.floor(length / 2) do
    array[length - index + 1], array[index] = array[index], array[length - index + 1]
  end
end

local function rotated(array, amount)
  local result <const> = {}
  local length <const> = #array
  amount = amount % length
  -- Don't bail out for zero amount, since we need to copy the array!
  if amount < 0 then -- fix the amount if negative
    amount = amount + length
  end
  for index = 1, length do
    local value <const> = array[index]
    local j = index + amount
    if j > length then
      j = j - length
    end
    result[j] = value
  end
  return result
end

local function rotate(array, amount)
  local length <const> = #array
  amount = amount % length
  if amount == 0 then
    return
  end
  if amount < 0 then -- fix the amount if negative
    amount = amount + length
  end
  -- compute the least-common-multiple
  local a = length
  local lcm = amount
  local b = a % lcm
  while b ~= 0 do
    a = lcm
    lcm = b
    b = a % lcm
  end
  for i = 1, lcm do
    local aux = array[i]
    local j = i + amount
    while j ~= i do
      array[j], aux = aux, array[j]
      j = j + amount
      if j > length then
        j = j - length
      end
    end
    array[i] = aux
  end
end

local function shuffled(array) -- inside-out algorithm
  local result <const> = {}
  for i = 1, #array do
    local j <const> = math.random(i)
    if i ~= j then
      result[i] = result[j]
    end
    result[j] = array[i]
  end
  return result
end

local function shuffle(array)
  for i = #array, 2, -1 do
    local j <const> = math.random(i)
    array[j], array[i] = array[i], array[j]
  end
end

local function uniqued(array)
  local result <const> = {}
  local n = 0
  local length <const> = #array
  local previous = nil
  for index = 1, length do
    local current <const> = array[index]
    if not previous or previous ~= current then
      n = n + 1
      result[n] = current;
    end
    previous = current
  end
  return result
end

local function unique(array)
  local previous = nil
  for index = #array, 1, -1 do
    local current <const> = array[index]
    if previous and previous == current then
      table.remove(array, index)
    end
    previous = current
  end
end

local function new(length, value)
  local result <const> = {}
  for index = 1, length do
    result[index] = value or index - 1
  end
  return result
end

local function equals(a, b)
  if a == b then
    return true
  end
  if not a or not b then
    return false
  end
  if #a ~= #b then
    return false
  end
  for index = 1, #a do
    if a[index] ~= b[index] then
      return false
    end
  end
  return true
end

local function copy(array, from, to)
  local result <const> = {}
  local n = 0
  for index = from or 1, to or #array do
    n = n + 1
    result[n] = array[index]
  end
  return result
end

local function merge(a, b)
  local n = #a
  for index = 1, #b do
    n = n + 1
    a[n] = b[index]
  end
end

local function merged(a, b)
  local result <const> = {}
  local n = 0
  for index = 1, #a do
    n = n + 1
    result[n] = a[index]
  end
  for index = 1, #b do
    n = n + 1
    result[n] = b[index]
  end
  return result
end

local function split(array, index)
  local left <const>, right <const> = {}, {}
  for i, v in ipairs(array) do
    if i <= index then
      table.insert(left, v)
    else
      table.insert(right, v)
    end
  end
  return left, right
end

-- Naive implementation of insertion-sort which is stable and uber-efficient when the table is incrementally grown
-- and re-sorted every time (that is, only the last item is eventually moved to the correct place).
-- It's even faster than `table.sort()`.
--
-- Note that's not Cormen-Leiserson-Rivest's optimized version, since it won't work with Lua's `for ...`.
local function _lower_than(a, b)
  return a < b
end

local function sort_range(array, from, to, comparator)
  local lower_than <const> = comparator or _lower_than
  for i = from + 1, to do
    for j = i, from + 1, -1 do
      if not lower_than(array[j], array[j - 1]) then -- Preserve stability! Swap only if strictly lower-than!
        break
      end
      array[j - 1], array[j] = array[j], array[j - 1] -- Swap adjacent slots.
    end
  end
end

local function sort(array, comparator)
  return sort_range(array, 1, #array, comparator)
end

-- Add the element `item` to `array`, preserving the current ordering. Designed to be incrementally called to
-- obtain an ever-ordered array of elements.
local function add(array, item, comparator)
  local lower_than <const> = comparator or _lower_than
  for index, other in ipairs(array) do
    if lower_than(item, other) then
      table.insert(array, index, item)
      return
    end
  end
  table.insert(array, item)
end

return {
  erase_if = erase_if,
  displace = displace,
  generate = generate,
  index_of = index_of,
  reversed = reversed,
  reverse = reverse,
  rotated = rotated,
  rotate = rotate,
  shuffled = shuffled,
  shuffle = shuffle,
  uniqued = uniqued,
  unique = unique,
  new = new,
  equals = equals,
  copy = copy,
  merge = merge,
  merged = merged,
  split = split,
  sort_range = sort_range,
  sort = sort,
  add = add
}
