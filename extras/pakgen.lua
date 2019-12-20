--[[
  Copyright (c) 2019 Marco Lizza (marco.lizza@gmail.com)

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

-- Depends upon
--  0 bit32
--  1 luafilesystem
--  2 luazen
--  3 struct
--  2 zlib

local bit32 = require("bit32")
local lfs = require("lfs")
local luazen = require("luazen")
local struct = require("struct")

local ZERO_PADDING = string.char(0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00)

local VERSION = 0x0000

function string:at(index)
  return self:sub(index, index)
end

function string:starts_with(prefix)
  return self:sub(1, #prefix) == prefix
end

function string:ends_with(suffix)
  return self:sub(1 + self:len() - #suffix) == suffix
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

  output:write(struct.pack('c8', "TOFUPAK!"))
  output:write(struct.pack('I2', VERSION))
  output:write(struct.pack('I2', flags))
  output:write(struct.pack('I4', #files))
end

local function emit_entry(output, file, config)
  local input = io.open(file.pathfile, "rb")

  local content = input:read("*all")

  if config.encrypted then
    local key = file.name .. ZERO_PADDING
    content = luazen.rc4raw(content, key:sub(1, 16))
  end

  output:write(struct.pack('I2', 0x2123))
  output:write(struct.pack('I2', #file.name))
  output:write(struct.pack('I4', file.size))
  output:write(struct.pack("c0", file.name))
  output:write(content)

  input:close()
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

emit_header(output, config, files)
for index, file in ipairs(files) do
  emit_entry(output, file, config)

  print(string.format("  [%d] `%s` %d", index - 1, file.name, file.size))
end

output:close()
print("Done!")
