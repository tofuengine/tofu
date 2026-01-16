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

local function map(collection, callback) -- mapper(value, key, collection)
  local result = {}
  for k, v in pairs(collection) do
    result[k] = callback(v, k, collection)
  end
  return result
end

local function filter(collection, callback) -- filter(value, key, collection)
  local result = {}
  for k, v in pairs(collection) do
    if callback(v, k, collection) then
      result[k] = v
    end
  end
  return result
end

local function reduce(collection, callback, initial_value) -- reducer(accumulator, value, key, collection)
  local accumulator = initial_value
  for k, v in pairs(collection) do
    if not accumulator then
      accumulator = v
    else
      accumulator = callback(accumulator, v, k, collection)
    end
  end
  return accumulator
end

local function for_each(collection, callback) -- callback(value, key, collection)
  for k, v in pairs(collection) do
    callback(v, k, collection)
  end
end

local function every(collection, callback) -- callback(value, key, collection)
  for k, v in pairs(collection) do
    if not callback(v, k, collection) then
      return false, k
    end
  end
  return true, nil
end

local function some(collection, callback) -- callback(value, key, collection)
  for k, v in pairs(collection) do
    if callback(v, k, collection) then
      return true, k
    end
  end
  return false, nil
end

local function find(collection, callback) -- callback(value, key, collection)
  for k, v in pairs(collection) do
    if callback(v, k, collection) then
      return v, k
    end
  end
  return nil, nil
end

local function equals(a, b)
  if a == b then
    return true
  end
  if not a or not b then
    return false
  end
  for k, v in pairs(a) do
    if b[k] ~= v then
      return false
    end
  end
  for k, v in pairs(b) do
    if a[k] ~= v then
      return false
    end
  end
  return true
end

local function copy(collection, ...)
  local result <const> = {}
  local keys <const> = { ... }
  for _, k in ipairs(keys) do
    result[k] = collection[k]
  end
  return result
end

local function merge(a, b)
  for k, v in pairs(b) do
    a[k] = v
  end
end

local function merged(a, b)
  local result <const> = {}
  for k, v in pairs(a) do
    result[k] = v
  end
  for k, v in pairs(b) do
    result[k] = v
  end
  return result
end

local function read_only(collection)
  local proxy <const> = {}
  local mt <const> = {
      __index = collection,
      __newindex = function(t, _, _)
          error(string.format("attempt to update read-only table %s", t), 2)
        end
    }
  return setmetatable(proxy, mt)
end

return {
  map = map,
  filter = filter,
  reduce = reduce,
  for_each = for_each,
  every = every,
  some = some,
  find = find,
  equals = equals,
  copy = copy,
  merge = merge,
  merged = merged,
  read_only = read_only,
}
