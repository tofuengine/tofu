#!/usr/bin/lua5.3

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

local lfs = require("lfs")
local luazen = require("luazen")

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

local function parse_arguments(args)
  local config = {
      input = {},
      output = "aout.lua"
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
      if not config.output:ends_with(".lua") then
        config.output = config.output .. ".lua"
      end
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

local function emit_header(output)
  output:write([[
-- This file has been autogenerated.
local Storage = require("tofu.io.storage")

local FILES <const> = {
]])
end

local function emit_entry(output, file)
  local name = string.gsub(string.lower(file.name), "\\", "/") -- Fix Windows' path separators.

  local input = io.open(file.pathfile, "rb")
  local data = luazen.b64encode(input:read("*all"), 0)

  output:write(string.format("  [\"%s\"] = {\n", name))
  while #data > 0 do
    local lump = data:sub(1, 100)
    local is_last_one = #data <= 100
    output:write(string.format("    \"%s\"%s", lump, is_last_one and "\n" or ",\n"))
    data = data:sub(#lump + 1)
  end
  output:write(string.format("  },\n", name))

  input:close()
end

local function emit_trailer(output)
  output:write([[
}

for name, lumps in pairs(FILES) do
  local bytes = ""
  for _, lump in ipairs(lumps) do
    bytes = bytes .. lump
  end
  Storage.inject(name, bytes)
end
]])
end

local function emit(config, files)
  local output = io.open(config.output, "w")

  emit_header(output)
  for _, file in ipairs(files) do
    emit_entry(output, file)
  end
  emit_trailer(output)

  output:close()

  return true
end

local function main(arg)
  local config = parse_arguments(arg)
  if not config then
    print("Usage: loadgen --input=<input folder> --output=<output file>")
    return
  end

  print("LoadGen v0.1.0")
  print("==============")

  local files = fetch_files(config.input)

  print(string.format("Creating preload `%s` w/ %d entries", config.output, #files))

  local success = emit(config, files)

  if success then
    print("Done!")
  else
    print("Failed!")
  end
end

main(arg)
