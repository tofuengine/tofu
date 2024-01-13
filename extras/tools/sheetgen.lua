#!/usr/bin/env lua5.4

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

local argparse = require("argparse")

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

local function convert(output, flags, input)
  if not input:ends_with(".txt") then
    input = input .. ".txt"
  end
  if not output:ends_with(".sheet") then
    output = output .. ".sheet"
  end

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

  if not flags.quiet then
    print(string.format("  generating %d entries", #sheet))
  end

  local writer = io.open(output, "wb")
  if not writer then
    print(string.format("*** can't create file `%s`", output))
    return false
  end

  for index, entry in ipairs(sheet) do
    if not flags.quiet then
      print(string.format("  entry #%d `%s` at <%d, %d> w/ size %dx%d", index, entry.id, entry.x, entry.y, entry.width, entry.height))
    end

    writer:write(string.pack("<i4", entry.x))
    writer:write(string.pack("<i4", entry.y))
    writer:write(string.pack("<I4", entry.width))
    writer:write(string.pack("<I4", entry.height))
  end

  writer:close()

  return true
end

local function main(arg)
  -- https://argparse.readthedocs.io/en/stable/options.html#flags
  local parser = argparse()
    :name("sheetgen")
    :description("Sheet generator.")
  parser:argument("input")
    :description("Paths to the file to be converted.")
    :args("1")
  parser:option("-o --output")
    :description("Name of the the generated sheet file.")
    :default("aout.sheet")
    :count(1)
    :args(1)
  parser:flag("-q --quiet")
    :description("Enables quiet output during sheet generation.")
  local args = parser:parse(arg)

  local flags = {}
  for _, flag in ipairs({ "quiet" }) do
    flags[flag] = args[flag] and true or false
  end

  if not flags.quiet then
    print("SheetGen v0.2.0")
    print("===============")
  end

  if not flags.quiet then
    print(string.format("Converting file `%s` to `%s`", args.input, args.output))
  end

  local success = convert(args.output, flags, args.input)

  if not flags.quiet then
    if success then
      print("Done!")
    else
      print("Failed!")
    end
  end

  os.exit(not success and -1 or 0)
end

main(arg)
