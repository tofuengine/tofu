#!/usr/bin/lua5.3

--[[
MIT License

Copyright (c) 2019-2022 Marco Lizza

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

--[[
+---------+
| HEADER  | sizeof(Pak_Header_t)
+---------+
| BLOB 0  |
+---------+
| BLOB 1  |
+---------+
    ...
    ...
    ...
+---------+
| BLOB n  |
+---------+
| ENTRY 0 | sizeof(uint16_t) + N * sizeof(char) + sizeof(uint32_t) + sizeof(uint32_t)
+---------+
| ENTRY 1 |
+---------+
    ...
    ...
    ...
+---------+
| ENTRY n |
+---------+
|  INDEX  | sizeof(Pak_Index_t)
+---------+

]]


local lfs = require("lfs")
local luazen = require("luazen")

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

local function xor_cipher(key)
  local k = { string.byte(key, 1, 256) } -- Up to 256 bytes for the key.
  local n = #k
  local i = 1
  return function(data)
    local d = { string.byte(data, 1, -1) }
    local r = {}
    for _, b in ipairs(d) do
      table.insert(r, b ~ k[i])
      i = (i % n) + 1
    end
    return string.char(table.unpack(r))
  end
end

local function parse_arguments(args)
  local config = {
      input = {},
      output = nil,
      encrypted = false
    }
  for _, arg in ipairs(args) do
    if arg:starts_with("--input=") then
      local path = arg:sub(9)
      if path:ends_with("/") then
        path = path:sub(1, -2)
      end
      table.insert(config.input, path)
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

local function attrdir(path, files)
  for file in lfs.dir(path) do
      if file ~= "." and file ~= ".." then
        local pathfile = path .. "/" .. file
        local mode = lfs.attributes(pathfile, "mode")
        if mode == "directory" then
            attrdir(pathfile, files)
        else
          local size = lfs.attributes(pathfile, "size")
          table.insert(files, { pathfile = pathfile, size = size, name = nil })
        end
      end
  end
  return files
end

local function fetch_files(paths)
  local files = {}
  for _, path in ipairs(paths) do
    print(string.format("Fetching files from folder `%s`", path))
    for _, file in ipairs(attrdir(path, {})) do
      file.name = file.pathfile:sub(1 + #path + 1)
      table.insert(files, file)
    end
  end
  print(string.format("Optimizing..."))
  table.sort(files, function(lhs, rhs) return lhs.pathfile < rhs.pathfile end)
  return files
end


--[[

The `flags` field is a bitmask with the following significance:

+-------+----------------------------------+
+ BIT # | DESCRIPTION                      |
+-------+----------------------------------+
+    0  | the archive content is encrypted |
+  1-7  | unused                           |
+-------+----------------------------------+

]]
local function compile_flags(config)
  return bit32.lshift(config.encrypted and 1 or 0, 0)
end

--[[

+--------+------+-----------+-----------------------------+
+ OFFSET | SIZE | NAME      | DESCRIPTION                 |
+--------+------+-----------+-----------------------------+
+    0   |   8  | signature | file signature (`TOFUPAK!`) |
+    8   |   1  | version   | file-format version (`0`)   |
+    9   |   1  | flags     | archive flags (see above)   |
+   10   |   2  | padding   | reserved for future uses    |
+--------+------+-----------+-----------------------------+
         |  12  |
         +------+

]]
local function emit_header(output, config)
  output:write(string.pack("c8", "TOFUPAK!"))
  output:write(string.pack("I1", VERSION))
  output:write(string.pack("I1", compile_flags(config)))
  output:write(string.pack("I2", RESERVED_16b))

  return 8 + 1 + 1 + 2
end

local function emit_entry(output, config, file, offset)
  local name = string.gsub(string.lower(file.name), "\\", "/") -- Fix Windows' path separators.

  local id = luazen.md5(name)

  local cipher = config.encrypted and xor_cipher(id) or nil

  local input = io.open(file.pathfile, "rb")
  local size = 0
  while true do
    local block = input:read(8196)
    if not block then
      break
    end
    if cipher then
      block = cipher(block)
    end
    output:write(block)
    size = size + #block
  end
  input:close()

  return {
      id = string.to_hex(id),
      offset = offset,
      size = size,
      name = name,
      file = file
    }
end

local function emit_entries(output, config, files, offset)
  local size = 0
  local entries = {}
  local hash = {}

  for _, file in ipairs(files) do
    local entry = emit_entry(output, config, file, offset + size)

    if hash[entry.name] then -- Check whether the (normalized) entry name appears twice.
      print("*** entry w/ name `%s` is duplicated")
      return nil, size
    end
    hash[entry.name] = true

    print(string.format("> file `%s`, name: `%s`, id: `%s`, offset: %d, size: %d",
      file.name, entry.name, entry.id, entry.offset, entry.size))

    table.insert(entries, entry)
    size = size + entry.size
  end

  return entries, size
end

--[[

The directory precedes the index and it's formed by a sequence of entries. Each
entry has this format

+--------+------+---------+-----------------------------------------------------------+
+ OFFSET | SIZE | NAME    | DESCRIPTION                                               |
+--------+------+---------+-----------------------------------------------------------+
+    0   |   4  | offset  | location of the entry, from the beginning of the file     |
+    4   |   4  | size    | length (in bytes) of the entry                            |
+    8   |   2  | chars   | # of characters of the entry's name                       |
+   10   |   *  | name    | sequence of characters, not null-terminated               |
+--------+------+---------+-----------------------------------------------------------+
         | >10  |
         +------+

]]
local function emit_directory(output, entries)
  for _, entry in ipairs(entries) do
    output:write(string.pack("I4", entry.offset))
    output:write(string.pack("I4", entry.size))
    output:write(string.pack("s2", entry.name))
  end
end

--[[

At the very end of the archive, the `index` structure is located. It serves,
unsurprisingly, as and index to locate the archive directory.

+--------+------+---------+-----------------------------------------------------------+
+ OFFSET | SIZE | NAME    | DESCRIPTION                                               |
+--------+------+---------+-----------------------------------------------------------+
+    0   |   4  | offset  | location of the directory, from the beginning of the file |
+    4   |   4  | entries | # of entries present in the directory                     |
+--------+------+---------+-----------------------------------------------------------+
         |   8  |
         +------+

]]
local function emit_index(output, offset, entries)
  output:write(string.pack("I4", offset))
  output:write(string.pack("I4", entries))
end

--[[

The trailer is the combination of both the directory and the index.

]]
local function emit_trailer(output, entries, cursor)
  emit_directory(output, entries)
  emit_index(output, cursor, #entries)
end

local function emit(config, files)
  local output = io.open(config.output, "wb")

  local header_size = emit_header(output, config)

  local entries, entries_size = emit_entries(output, config, files, header_size)
  if not entries then
    output:close()
    os.remove(config.output)
    return false
  end

  emit_trailer(output, entries, header_size + entries_size)

  output:close()

  return true
end

local function main(arg)
  local config = parse_arguments(arg)
  if not config then
    print("Usage: pakgen --input=<input folder> --output=<output file> [--encrypted]")
    return
  end

  print("PakGen v0.4.1")
  print("=============")

  local flags = {}
  if config.encrypted then
    table.insert(flags, "encrypted")
  end
  local annotation = #flags == 0 and "plain" or table.concat(flags, " and ")

  local files = fetch_files(config.input)

  print(string.format("Creating %s archive `%s` w/ %d entries", annotation, config.output, #files))

  local success = emit(config, files)

  if success then
    print("Done!")
  else
    print("Failed!")
  end
end

main(arg)
