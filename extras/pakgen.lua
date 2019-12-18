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
--  1 luafilesystem
--  2 struct

local struct = require("struct")
local lfs = require("lfs")

function string:at(index)
  return self:sub(index, index)
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

local function emit_header(output, flags, files)
  output:write(struct.pack('c8', "TOFUPAK!"))
  output:write(struct.pack('I4', 0x00000100))
  output:write(struct.pack('I4', 0x00000000))
  output:write(struct.pack('I4', #files))
end

local function emit_entry_header(output, file)
  output:write(struct.pack('i4', file.offset))
  output:write(struct.pack('I4', file.size))
  output:write(struct.pack('I4', #file.name))
  output:write(struct.pack("c0", file.name))
end

local function emit_entry(output, file)
  local input = io.open(file.pathfile, "rb")
  output:write(input:read("*all"))
  input:close()
end

if #arg ~= 2 then
  print("Usage: pak <input folder> <output file>")
  return
end

local input_folder = arg[1]
if input_folder:ends_with("/") then
  input_folder = input_folder:sub(1, -2)
end

local output_file = arg[2]
if not output_file:ends_with(".pak") then
  output_file = output_file .. ".pak"
end

local files = {}
attrdir(input_folder, files)
table.sort(files, function(lhs, rhs) return lhs.pathfile < rhs.pathfile end)

local offset = 20
for _, file in ipairs(files) do
  file.name = file.pathfile:sub(1 + #input_folder + 1)
  offset = offset + (12 + #file.name)
end

for _, file in ipairs(files) do
  file.offset = offset
  print(file.pathfile .. " " .. file.name .. " " .. file.offset .. " " .. file.size)
  offset = offset + file.size
end

local output = io.open(output_file, "wb")

emit_header(output, 0, files)
for _, file in ipairs(files) do
  emit_entry_header(output, file)
end
for _, file in ipairs(files) do
  emit_entry(output, file)
end

output:close()