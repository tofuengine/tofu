#!/usr/bin/lua5.4

--[[
MIT License

Copyright (c) 2019-2023 Marco Lizza

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
--  1 argparse
--  2 luafilesystem
--  3 luazen

--[[
+---------+
| HEADER  | sizeof(Pak_Header_t)
+---------+
| BLOB 0  | N * sizeof(char)
+---------+
| BLOB 1  |    "        "
+---------+
    ...
    ...
    ...
+---------+
| BLOB n  |    "        "
+---------+
| ENTRY 0 | sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint16_t) + N * sizeof(char)
+---------+
| ENTRY 1 |    "                                                                 "
+---------+
    ...
    ...
    ...
+---------+
| ENTRY n |    "                                                                 "
+---------+
|  INDEX  | sizeof(Pak_Index_t)
+---------+

NOTE: `uint16_t` and `uint32_t` data is explicitly stored in little-endian.
      See the `<` modifier below.

]]

local argparse = require("argparse")
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

local function attrdir(path, files)
  if path:ends_with("/") then
    path = path:sub(1, -2)
  end

  local mode = lfs.attributes(path, "mode")
  if mode == "file" then
    local size = lfs.attributes(path, "size")
    table.insert(files, { pathfile = path, size = size, name = nil })
    return files
  end

  for entry in lfs.dir(path) do
    if entry ~= "." and entry ~= ".." then
      local subpath = path .. "/" .. entry
      attrdir(subpath, files)
    end
  end
  return files
end

local function fetch_files(paths, flags)
  local files = {}
  for _, path in ipairs(paths) do
    if not flags.quiet then
      print(string.format("Fetching files from path `%s`", path))
    end
    for _, file in ipairs(attrdir(path, {})) do
      local i, _ = string.find(path, "/") -- Check if the path is a straight current-directory file.
      file.name = not i and path or file.pathfile:sub(1 + #path + 1)
      table.insert(files, file)
    end
  end
  if not flags.quiet then
    print(string.format("Optimizing..."))
  end
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
local function compile_flags(flags)
  return flags.encrypted and 1 or 0 << 0
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
local function emit_header(writer, flags)
  writer:write(string.pack("c8", "TOFUPAK!"))
  writer:write(string.pack("I1", VERSION))
  writer:write(string.pack("I1", compile_flags(flags)))
  writer:write(string.pack("<I2", RESERVED_16b))

  return 8 + 1 + 1 + 2
end

local function emit_entry(writer, flags, file, offset)
  local name = string.gsub(string.lower(file.name), "\\", "/") -- Fix Windows' path separators.

  local id = luazen.md5(name)

  local cipher = flags.encrypted and xor_cipher(id) or nil

  local reader = io.open(file.pathfile, "rb")
  if not reader then
    print(string.format("*** can't access file `%s`", file.pathfile))
    return nil
  end

  local size = 0
  while true do
    local block = reader:read(8196)
    if not block then
      break
    end
    if cipher then
      block = cipher(block)
    end
    writer:write(block)
    size = size + #block
  end
  reader:close()

  return {
      id = string.to_hex(id),
      offset = offset,
      size = size,
      name = name,
      file = file
    }
end

local function emit_entries(writer, flags, files, offset)
  local size = 0
  local entries = {}
  local hash = {}

  for index, file in ipairs(files) do
    local entry = emit_entry(writer, flags, file, offset + size)
    if not entry then
      return nil, 0
    end

    if hash[entry.id] then -- Check whether the (normalized) entry name appears twice.
      print(string.format("*** entry w/ name `%s` is duplicated (clashing id is `%s`)", entry.name, entry.id))
      return nil, size
    end
    hash[entry.id] = true

    if not flags.quiet then
      if flags.detailed then
        print(string.format("> file `%s`\n  name: `%s`\n  id: `%s`\n  offset: %d\n  size: %d",
          file.name, entry.name, entry.id, entry.offset, entry.size))
      else
        print(string.format("[%04x] `%s` -> `%s`",
          index, entry.id, entry.name))
      end
    end

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
    output:write(string.pack("<I4", entry.offset))
    output:write(string.pack("<I4", entry.size))
    output:write(string.pack("<s2", entry.name))
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
local function emit_index(writer, offset, entries)
  writer:write(string.pack("<I4", offset))
  writer:write(string.pack("<I4", entries))
end

--[[

The trailer is the combination of both the directory and the index.

]]
local function emit_trailer(writer, entries, cursor)
  emit_directory(writer, entries)
  emit_index(writer, cursor, #entries)
end

local function emit(output, flags, files)
  local writer = io.open(output, "wb")
  if not writer then
    print(string.format("*** can't create file `%s`", output))
    return false
  end

  local header_size = emit_header(writer, flags)

  local entries, entries_size = emit_entries(writer, flags, files, header_size)
  if not entries then
    writer:close()
    os.remove(output)
    return false
  end

  emit_trailer(writer, entries, header_size + entries_size)

  writer:close()

  return true
end

local function main(arg)
  -- https://argparse.readthedocs.io/en/stable/options.html#flags
  local parser = argparse()
    :name("pakgen")
    :description("Package generator.")
  parser:argument("input")
    :description("Paths to be added to the package. Can be either single files or directories (which are recursively scanned).")
    :args("+")
  parser:option("-o --output")
    :description("Name of the the generated package file.")
    :default("aout.pak")
    :count(1)
    :args(1)
  parser:flag("-q --quiet")
    :description("Enables quiet output during package creation.")
  parser:flag("-d --detailed")
    :description("Enables detailed output during package creation.")
  parser:flag("-e --encrypted")
    :description("Tells whether the package should be encrypted.")
  local args = parser:parse(arg)

  local flags = {}
  for _, flag in ipairs({ "quiet", "detailed", "encrypted" }) do
    flags[flag] = args[flag] and true or false
  end

  if not flags.quiet then
    print("PakGen v0.5.1")
    print("=============")
  end

  local files = fetch_files(args.input, flags)

  if not flags.quiet then
    local annotation = flags.encrypted and "encrypted" or "plain"
    print(string.format("Creating %s archive `%s` w/ %d entries", annotation, args.output, #files))
  end

  local success = emit(args.output, flags, files)

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
