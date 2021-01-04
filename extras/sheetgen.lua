#!/usr/bin/lua5.3

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

-- Depends upon the following Lua "rocks".
--  - lua-struct

local struct = require("struct")

function string:at(index)
  return self:sub(index, index)
end

function string:starts_with(prefix)
  return self:sub(1, #prefix) == prefix
end

function string:ends_with(suffix)
  return self:sub(1 + self:len() - #suffix) == suffix
end

function string.from_hex(str)
  return str:gsub('..', function(cc)
        return string.char(tonumber(cc, 16))
    end)
end

function string.to_hex(str)
  return str:gsub('.', function(c)
        return string.format('%02X', string.byte(c))
    end)
end

local function convert(output, input)
  local sheet = {}
  for line in io.lines(input) do
    -- i0.png, 0, 0, 15, 21, 0, 0, 0, 0
    local id, x, y, width, height = line:match("([^,]+),%s*(%d+),%s*(%d+),%s*(%d+),%s*(%d+),.+")
    table.insert(sheet, {
        id = id,
        x = tonumber(x),
        y = tonumber(y),
        width = tonumber(width),
        height = tonumber(height)
      })
  end

  print(string.format("  generating %d entries", #sheet))

  local out = io.open(output, "wb")
  for index, entry in ipairs(sheet) do
    print(string.format("  entry #%d `%s` at <%d, %d> w/ size %dx%d", index, entry.id, entry.x, entry.y, entry.width, entry.height))
    out:write(struct.pack("I", entry.x))
    out:write(struct.pack("I", entry.y))
    out:write(struct.pack("I", entry.width))
    out:write(struct.pack("I", entry.height))
  end
  out:close()
end

local function parse_arguments(args)
  local config = {
      input = nil,
      output = nil
    }
  for _, arg in ipairs(args) do
    if arg:starts_with("--input=") then
      config.input = arg:sub(9)
      if not config.input:ends_with(".txt") then
        config.input = config.input .. ".txt"
      end
    elseif arg:starts_with("--output=") then
      config.output = arg:sub(10)
      if not config.output:ends_with(".sheet") then
        config.output = config.output .. ".sheet"
      end
    end
  end
  return (config.input and config.output) and config or nil
end

local function main(arg)
  local config = parse_arguments(arg)
  if not config then
    print("Usage: sheetgen --input=<input file> --output=<output file>")
    return
  end

  print("SheetGen v0.1.0")
  print("===============")

  print(string.format("Converting file `%s` to `%s`", config.input, config.output))
  convert(config.output, config.input)

  print("Done!")
end

main(arg)