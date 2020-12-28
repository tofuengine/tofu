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

-- Depends upon the following Lua "rocks".
--  1 luafilesystem
--  2 luazen
--  3 lua-struct

local lfs = require("lfs")
local luazen = require("luazen")
local struct = require("struct")

local VERSION = 0x00
local RESERVED_16b = 0xFFFF

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

local function attrdir(path, list)
  for file in lfs.dir(path) do
      if file ~= "." and file ~= ".." then
        local pathfile = path .. "/" .. file
        local mode = lfs.attributes(pathfile, "mode")
        if mode == "directory" then
            attrdir(pathfile, list)
        else
          local size = lfs.attributes(pathfile, "size")
          table.insert(list, { pathfile = pathfile, size = size, name = nil, offset = -1 })
        end
      end
  end
end

local function emit_header(output, config, files)
  local flags = bit32.lshift(config.encrypted and 1 or 0, 0)

  output:write(struct.pack("c8", "TOFUPAK!"))
  output:write(struct.pack("B", VERSION))
  output:write(struct.pack("B", flags))
  output:write(struct.pack("H", RESERVED_16b))

  return 8 + 1 + 1 + 2
end

local function emit_entry(output, file, config, cursor)
  local input = io.open(file.pathfile, "rb")
  local content = input:read("*all")
  input:close()

  if config.encrypted then
    file.name = luazen.md5(file.name) -- TODO: check for collisions.
    content = luazen.rc4raw(content, file.name)
  end

  output:write(content)

  file.offset = cursor -- Se the file offset in the archive.

  return cursor + #content
end

local function emit_directory(output, files)
  for _, file in ipairs(files) do
    output:write(struct.pack("H", RESERVED_16b))
    output:write(struct.pack("H", #file.name))
    output:write(struct.pack("I", file.offset))
    output:write(struct.pack("I", file.size))
    output:write(struct.pack("c" .. #file.name, file.name))
  end
end

local function emit_trailer(output, files, cursor)
  emit_directory(output, files)
  output:write(struct.pack("I", cursor))
  output:write(struct.pack("I", #files))
end

local function parse_arguments(args)
  local config = {
      input = nil,
      output = nil,
      encrypted = false
    }
  for _, arg in ipairs(args) do
    if arg:starts_with("--input=") then
      config.input = arg:sub(9)
      if config.input:ends_with("/") then
        config.input = config.input:sub(1, -2)
      end
    elseif arg:starts_with("--output=") then
      config.output = arg:sub(10)
      if not config.output:ends_with(".pak") then
        config.output = config.output .. ".pak"
      end
    elseif arg:starts_with("--encrypted") then
      config.encrypted = true
    end
  end
  return (config.input and config.output) and config or nil
end

local function fetch_files(path)
  local files = {}
  attrdir(path, files)
  table.sort(files, function(lhs, rhs) return lhs.pathfile < rhs.pathfile end)
  for _, file in ipairs(files) do
    file.name = file.pathfile:sub(1 + #path + 1)
  end
  return files
end

local function main(arg)
  local config = parse_arguments(arg)
  if not config then
    print("Usage: pakgen --input=<input folder> --output=<output file> [--encrypted]")
    return
  end

  local flags = {}
  if config.encrypted then
    table.insert(flags, "encrypted")
  end
  local annotation = #flags == 0 and "plain" or table.concat(flags, " and ")

  local files = fetch_files(config.input)

  print(string.format("Creating %s archive `%s` w/ %d entries", annotation, config.output, #files))
  local output = io.open(config.output, "wb")

  local cursor = emit_header(output, config, files)
  for index, file in ipairs(files) do
    cursor = emit_entry(output, file, config, cursor)

    print(string.format("  [%d] `%s` %d", index - 1, file.name, file.size))
  end
  emit_trailer(output, files, cursor)

  output:close()
  print("Done!")
end

main(arg)
