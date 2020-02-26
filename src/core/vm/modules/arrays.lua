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

-- http://developer.classpath.org/doc/java/util/Collections-source.html

-- TODO: rename as `Collections`?

--[[

function collections.merge(target, source)
  for key, value in pairs(source) do
    if type(value) == "table" then
      local merged = target[key] or {}
      merge(merged, value)
      target[key] = merged
    else
      target[key] = value
    end
  end
end

]]


local function map(array, callback) -- mapper(value, index, length, array)
    local result = {}
    local length = #array
    for index = 1, length do
      local value = array[index]
      result[index] = callback(value, index, length, array)
    end
    return result
  end
  
  local function filter(array, callback) -- filter(value, index, length, array)
    local result = {}
    local length = #array
    local n = 0
    for index = 1, length do
      local value = array[index]
      if callback(value, index, length, array) then
        n = n + 1
        result[n] = value
      end
    end
    return result
  end
  
  local function reduce(array, callback, initial_value) -- reducer(accumulator, value, index, length, array)
    local accumulator = initial_value
    local length = #array
    for index = 1, length do
      local value = array[index]
      if accumulator == nil then
        accumulator = value
      else
        accumulator = callback(accumulator, value, index, length, array)
      end
    end
    return accumulator
  end
  
  local function for_each(array, callback) -- callback(value, index, length, array)
    local length = #array
    for index = 1, length do
      local value = array[index]
      callback(value, index, length, array)
    end
  end
  
  local function every(array, callback) -- callback(value, index, length, array)
    local length = #array
    for index = 1, length do
      local value = array[index]
      if not callback(value, index, length, array) then
        return false, index
      end
    end
    return true, nil
  end
  
  local function some(array, callback) -- callback(value, index, length, array)
    local length = #array
    for index = 1, length do
      local value = array[index]
      if callback(value, index, length, array) then
        return true, index
      end
    end
    return false, nil
  end
  
  local function find(array, callback) -- callback(value, index, length, array)
    local length = #array
    for index = 1, length do
      local value = array[index]
      if callback(value, index, length, array) then
        return value, index
      end
    end
    return nil, nil
  end
  
  local function erase_if(array, callback) -- callback(value, index, length, array)
    local erased = 0
    local length = #array
    for index = length, 1, -1 do
      local value = array[index]
      local erase, stop = callback(value, index, length, array)
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
    local result = {}
    local length = #array
    for index = 1, length do
      local value = array[index]
      result[callback(value, index, length, array)] = value
    end
    return result
  end
  
  local function generate(callback)
    local result = {}
    local index = 1
    while true do
      local value = callback(index)
      if value == nil then
        break
      end
      result[index] = value
      index = index + 1
    end
    return result
  end
  
  local function index_of(array, search, from)
    local length = #array
    for index = from or 1, length do
      local value = array[index]
      if value == search then
        return index
      end
    end
    return 0
  end
  
  local function reversed(array)
    local result = {}
    local length = #array
    for index = 1, length do
      result[length - index + 1] = array[index]
    end
    return result
  end
  
  local function reverse(array)
    local length = #array
    for index = 1, math.floor(length / 2) do
      array[length - index + 1], array[index] = array[index], array[length - index + 1]
    end
  end
  
  local function rotated(array, amount)
    local result = {}
    local length = #array
    amount = amount % length
    -- Don't bail out for zero amount, since we need to copy the array!
    if amount < 0 then -- fix the amount if negative
      amount = amount + length
    end
    for index = 1, length do
      local value = array[index]
      local j = index + amount
      if j > length then
        j = j - length
      end
      result[j] = value
    end
    return result
  end
  
  local function rotate(array, amount)
    local length = #array
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
    local result = {}
    for i = 1, #array do
      local j = math.random(i)
      if i ~= j then
        result[i] = result[j]
      end
      result[j] = array[i]
    end
    return result
  end
  
  local function shuffle(array)
    for i = #array, 2, -1 do
      local j = math.random(i)
      array[j], array[i] = array[i], array[j]
    end
  end
  
  local function unique(array)
    -- TODO: implement a duplicate removal.
    return array
  end
  
  local function new(length, value)
    local result = {}
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
    local result = {}
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
    local result = {}
    local n = 0
    for index = 1, #a do
      n = n + 1
      a[n] = a[index]
    end
    for index = 1, #b do
      n = n + 1
      a[n] = b[index]
    end
    return result
  end
  
  return {
    map = map,
    filter = filter,
    reduce = reduce,
    for_each = for_each,
    every = every,
    some = some,
    find = find,
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
    unique = unique,
    new = new,
    equals = equals,
    copy = copy,
    merge = merge,
    merged = merged
  }