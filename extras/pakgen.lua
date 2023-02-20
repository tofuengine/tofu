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
| ENTRY 0 | sizeof(Pak_Entry_t)
+---------+
| ENTRY 1 |     "         "
+---------+
    ...
    ...
    ...
+---------+
| ENTRY n |     "         "
+---------+
| DATA 0  | sizeof(Entry) * sizeof(uint8_t)
+---------+
| DATA 1  |     "                     "
+---------+
    ...
    ...
    ...
+---------+
| DATA n  |     "                     "
+---------+

NOTE: `uint16_t` and `uint32_t` data are explicitly stored in little-endian.
      See the `<` modifier below.

]]

local argparse = require("argparse")
local lfs = require("lfs")
local luazen = require("luazen")

local HEADER_SIZE <const> = 8 + 1 + 1 + 2 + 4
local ENTRY_HEADER_SIZE <const> = 16 + 4 + 4

local VERSION <const> = 0x00
local RESERVED_16b <const> = 0xFFFF

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

local function null_cipher(_)
  return function(data)
    return data
  end
end

local function attrdir(path, name, files)
  local mode = lfs.attributes(path, "mode")
  if mode == "file" then
    local size = lfs.attributes(path, "size")
    table.insert(files, { pathfile = path, size = size, name = not name and path or name })
    return files
  end

  for entry in lfs.dir(path) do
    if entry ~= "." and entry ~= ".." then
      local subpath = path .. "/" .. entry
      local subname = not name and entry or name .. "/" .. entry
      attrdir(subpath, subname, files)
    end
  end
  return files
end

local function optimize_files(flags, files)
  local hash = {}

  if not flags.quiet then
    print(string.format("Optimizing..."))
  end

  local offset = HEADER_SIZE + #files * ENTRY_HEADER_SIZE

  for _, file in ipairs(files) do
    local name = string.gsub(string.lower(file.name), "\\", "/") -- Fix Windows' path separators.
    local id = luazen.md5(name)

    if hash[id] then -- Check whether the (normalized) entry name appears twice.
      print(string.format("*** entry w/ name `%s` is duplicated (id `%s` already used for `%s`)", file.name, string.to_hex(id), hash[id]))
      return false
    end
    hash[id] = file.name

    file.id = luazen.md5(file.name)
    file.offset = offset

    offset = offset + file.size
  end

  return true
end

local function fetch_files(paths, flags)
  local files = {}
  for _, path in ipairs(paths) do
    if not flags.quiet then
      print(string.format("Fetching files from path `%s`", path))
    end

    for _, file in ipairs(attrdir(path, nil, {})) do
      table.insert(files, file)
    end
  end
  return files
end

--[[

The `flags` field is a bitmask with the following significance:

+-------+----------------------------------+
+ BIT # | DESCRIPTION                      |
+-------+----------------------------------+
+    0  | the archive content is encrypted |
+    1  | the archive content is sorted    |
+  2-7  | unused                           |
+-------+----------------------------------+

]]
local function compile_flags(flags)
  return (flags.encrypted and 1 or 0) << 0
    | (flags.sorted and 1 or 0) << 1
end

--[[

+--------+------+-----------+-----------------------------+
+ OFFSET | SIZE | NAME      | DESCRIPTION                 |
+--------+------+-----------+-----------------------------+
+    0   |   8  | signature | file signature (`TOFUPAK!`) |
+    8   |   1  | version   | file-format version (`0`)   |
+    9   |   1  | flags     | archive flags (see above)   |
+   10   |   2  | padding   | reserved for future uses    |
+   10   |   4  | entries   | number of entries           |
+--------+------+-----------+-----------------------------+
         |  16  |
         +------+

]]
local function emit_header(writer, flags, files)
  writer:write(string.pack("c8", "TOFUPAK!"))
  writer:write(string.pack("I1", VERSION))
  writer:write(string.pack("I1", compile_flags(flags)))
  writer:write(string.pack("<I2", RESERVED_16b))
  writer:write(string.pack("I4", #files))

  return true
end

--[[

Each archive entry is formed by an header telling the size and the name of the entry,
follower by its content, as a sequence of bytes.

Note that the offset of the entry in the file is not stored, as it is automatically
calculated during the indexing process.

+--------+--------+----------+-----------------------------------------------------------+
+ OFFSET |  SIZE  | NAME     | DESCRIPTION                                               |
+--------+--------+----------+-----------------------------------------------------------+
+    0   |   16   | id       | entry ID (MD5 of the filename)                            |
+   16   |    4   | offset   | absolute position of the entry within the archive         |
+   20   |    4   | size     | length (in bytes) of the entry                            |
+--------+--------+----------+-----------------------------------------------------------+
         |   24   |
         +--------+

]]
local function emit_directory(writer, flags, files)
  -- Create a copy, we don't want to sort the files table so that we don't mess
  -- with the output order.
  local directory = {}
  for i, v in ipairs(files) do directory[i] = v end

  if flags.sorted then
    if not flags.quiet then
      print(string.format("Sorting..."))
    end
    table.sort(directory, function(lhs, rhs) return lhs.id < rhs.id end)
  end

  for index, entry in ipairs(directory) do
    if not flags.quiet then
      print(string.format("[%04x] `%s` -> `%s`",
        index, string.to_hex(entry.id), entry.name))
    end

    writer:write(string.pack("c16", entry.id))
    writer:write(string.pack("<I4", entry.offset))
    writer:write(string.pack("<I4", entry.size))
  end

  return true
end

local function emit_entry(writer, flags, file)
  local reader = io.open(file.pathfile, "rb")
  if not reader then
    print(string.format("*** can't access file `%s`", file.pathfile))
    return false
  end

  local key <const> = luazen.md5(file.id)
  local cipher = flags.encrypted and xor_cipher(key) or null_cipher(key)

  while true do
    local block = reader:read(8196)
    if not block then
      break
    end
    writer:write(cipher(block))
  end
  reader:close()

  return true
end

local function emit_entries(writer, flags, files)
  if not flags.quiet then
    print(string.format("Writing..."))
  end

  for index, file in ipairs(files) do
    local offset <const> = writer:seek() --"cur", 0)

    local done = emit_entry(writer, flags, file)
    if not done then
      return false
    end

    if not flags.quiet then
      if flags.detailed then
        print(string.format("> file `%s`\n  name: `%s`\n  id: `%s`\n  offset: %d\n  size: %d",
          file.pathfile, file.name, string.to_hex(file.id), offset, file.size))
      else
        print(string.format("[%04x] `%s` -> `%s`",
          index, string.to_hex(file.id), file.name))
      end
    end
  end

  return true
end

local function emit(output, flags, files)
  local optimized = optimize_files(flags, files)
  if not optimized then
    return false
  end

  local writer = io.open(output, "wb")
  if not writer then
    print(string.format("*** can't create file `%s`", output))
    return false
  end

  local header_done = emit_header(writer, flags, files)
  if not header_done then
    writer:close()
    os.remove(output)
    return false
  end

  local directory_done = emit_directory(writer, flags, files)
  if not directory_done then
    writer:close()
    os.remove(output)
    return false
  end

  local entries_done = emit_entries(writer, flags, files)
  if not entries_done then
    writer:close()
    os.remove(output)
    return false
  end

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
  parser:flag("-s --sorted")
    :description("Tells whether the package should be sorted.")
  local args = parser:parse(arg)

  local flags = {}
  for _, flag in ipairs({ "quiet", "detailed", "encrypted", "sorted" }) do
    flags[flag] = args[flag] and true or false
  end

  if not flags.quiet then
    print("PakGen v0.7.0")
    print("=============")
  end

  local files = fetch_files(args.input, flags)

  if not flags.quiet then
    local optimization = flags.sorted and "sorted" or "sorted"
    local annotation = flags.encrypted and "encrypted" or "plain"
    print(string.format("Creating %s %s archive `%s` w/ %d entries", optimization, annotation, args.output, #files))
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
