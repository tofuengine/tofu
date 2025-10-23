#!/usr/bin/env lua5.4

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

Copyright (c) 2019-2025 Marco Lizza

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
--  1 lua-vips
--  2 argparse

local argparse = require("argparse")
local vips = require("vips")

local function load_image(path, flags)
  if not flags.quiet then
    print(string.format("Loading image %s...", path))
  end
  local image = vips.Image.new_from_file(path, { access = "random" })
  if image:bands() == 3 then
      image = image:bandjoin({255})
  end
  image = image:cast("uchar")

  if not flags.quiet then
    print(string.format("Image size: %dx%d", image:width(), image:height()))
    print(string.format("Image bands: %d", image:bands()))
  end

  return image
end

-- Iterate over each pixel in the image, calling the given callback with
-- the pixel coordinates and RGBA values.
-- If the callback returns false, the iteration is stopped.
-- Returns true if the iteration completed, false otherwise.
local function for_each_pixel(image, callback)
  local width, height = image:width(), image:height()
  local buffer = image:write_to_memory() -- Conver to to a buffer for faster access.

  local offset = 0
  for y = 0, height - 1 do
      for x = 0, width - 1 do
          local r = buffer[offset + 0]
          local g = buffer[offset + 1]
          local b = buffer[offset + 2]
          local a = buffer[offset + 3]

          local result = callback(x, y, r, g, b, a)
          if not result then
            return false
          end

          offset = offset + 4
      end
  end
  return true
end

local function to_rgba32(r, g, b)
    return (r << 16) | (g << 8) | g
end

local function from_rgba32(rgb)
    local r = (rgb >> 16) & 0xff
    local g = (rgb >> 8) & 0xff
    local b = (rgb >> 0) & 0xff

    return r, g, b
end

local function length(T)
  local count = 0
  for _, _ in pairs(T) do count = count + 1 end
  return count
end

-- Given a image buffer in RGBA format, calculates the palette by scanning
-- the image pixels. For each new color found, a new entry is added to the
-- palette.
-- Returns the palette as a map (rgb -> index) and the palette length.
--
-- TODO: implement a better palette generation algorithm (like median cut).
--       See: https://en.wikipedia.org/wiki/Median_cut
local function table_split(tbl, index)
    local t1, t2 = {}, {}  -- create 2 new tables
    for i, v in ipairs(tbl) do
        if i <= index then
            table.insert(t1, v)
        else
            table.insert(t2, v)
        end
    end
    return t1, t2
end

local function _lower_than(a, b)
  return a < b
end

local function sort_range(array, from, to, comparator)
  local lower_than = comparator or _lower_than
  for i = from + 1, to do
    for j = i, from + 1, -1 do
      if not lower_than(array[j], array[j - 1]) then -- Preserve stability! Swap only if strictly lower-than!
        break
      end
      array[j - 1], array[j] = array[j], array[j - 1] -- Swap adjacent slots.
    end
  end
end

local function subtable_sort(table, from, to)
    table.sort(table, function(a, b)
        return a[from] < b[from]
    end)
end

-- Recursive median cut algorithm to split colors into clusters.
-- We don't need to create sub-tables, as we can work in-place in the
-- array of pixels (i.e. the image buffer).
local function median_cut(pixels, depth, clusters)
  if depth == 0 or #pixels == 0 then
    print(string.format("Reached leaf with %d pixels", #pixels))
    table.insert(clusters, pixels)
    return
  end

  local min_r, min_g, min_b = 255, 255, 255
  local max_r, max_g, max_b = 0, 0, 0
  for _, pixel in ipairs(pixels) do
      if pixel.r < min_r then min_r = pixel.r end
      if pixel.g < min_g then min_g = pixel.g end
      if pixel.b < min_b then min_b = pixel.b end
      if pixel.r > max_r then max_r = pixel.r end
      if pixel.g > max_g then max_g = pixel.g end
      if pixel.b > max_b then max_b = pixel.b end
  end
  local range_r = max_r - min_r
  local range_g = max_g - min_g
  local range_b = max_b - min_b
  print(string.format("Color ranges: R=%d, G=%d, B=%d", range_r, range_g, range_b))
  if range_r >= range_g and range_r >= range_b then
    print("Sorting by R")
    table.sort(pixels, function(a, b) return a.r < b.r end)
  elseif range_g >= range_r and range_g >= range_b then
    print("Sorting by G")
    table.sort(pixels, function(a, b) return a.g < b.g end)
  else
    print("Sorting by B")
    table.sort(pixels, function(a, b) return a.b < b.b end)
  end

  local left, right = table_split(pixels, math.floor(#pixels / 2))
  print(string.format("Split %d into %d and %d pixels", #pixels, #left, #right))

  median_cut(left, depth - 1, clusters)
  median_cut(right, depth - 1, clusters)
end

local function calculate_palette(image, colors)
  local pixels = {}
  local result = for_each_pixel(image,
    function(x, y, r, g, b, a)
        table.insert(pixels, { r = r, g = g, b = b, a = a })
        return true
    end)

  local depth = math.tointeger(math.ceil(math.log(colors) / math.log(2)))
  print(string.format("Calculating palette with depth %d...", depth))
  local clusters = {}
  median_cut(pixels, depth, clusters)
  print(string.format("Calculated %d color clusters", #clusters))

  local palette = {}
  local palette_length = 0

  for _, cluster in ipairs(clusters) do
      local r_sum, g_sum, b_sum = 0, 0, 0
      for _, pixel in ipairs(cluster) do
          r_sum = r_sum + pixel.r
          g_sum = g_sum + pixel.g
          b_sum = b_sum + pixel.b
      end

      local count = #cluster
      local r = math.tointeger(math.floor(r_sum / count + 0.5))
      local g = math.tointeger(math.floor(g_sum / count + 0.5))
      local b = math.tointeger(math.floor(b_sum / count + 0.5))

      local rgb = to_rgba32(r, g, b)
      if not palette[rgb] then
        if palette_length == 256 then
          print("*** too many colors in the image (max 256)")
          return false
        end

        print(string.format("new palette entry found: %08x", rgb))
        palette[rgb] = palette_length -- Store the palette index for this color.
        palette_length = palette_length + 1
      end
  end

  if not result then
    return nil
  end

  return palette, palette_length
end

local function emit_header(writer, flags, image, palette)
  local width, height = image:width(), image:height()
  local palette_length = length(palette)

  -- `TOFUIMG!` (8 bytes, ASCII)
  -- width (2 bytes, little-endian)
  -- height (2 bytes, little-endian)
  -- flags (2 bytes, little-endian)
  --   bit 0: RLE encoding (0 = no, 1 = yes)
  -- palette length (2 bytes, little-endian)
  -- palette data (palette size * 3 bytes)
  writer:write(string.pack("c8", "TOFUIMG!"))
  writer:write(string.pack("<I2", width))
  writer:write(string.pack("<I2", height))
  writer:write(string.pack("<I2", 0)) -- No RLE encoding for now.
  writer:write(string.pack("<I2", palette_length))
  for rgb, index in pairs(palette) do
    local r, g, b = from_rgba32(rgb)
    writer:write(string.pack("BBB", r, g, b))
  end
end

local function match_color_in_palette(r, g, b, palette)
  local best_index = 0
  local best_distance = math.huge
  for crgb, index in pairs(palette) do
    local cr, cg, cb = from_rgba32(crgb)
    local dr = r - cr
    local dg = g - cg
    local db = b - cb
    local distance = (dr * dr) + (dg * dg) + (db * db)
    if distance < best_distance then
      best_distance = distance
      best_index = index
    end
  end
  return best_index
end

local function emit_data(writer, flags, image, palette)
  -- pixel data (width * height bytes, each byte is an index in the palette)
  local result = for_each_pixel(image,
    function(x, y, r, g, b, a)
        local rgb = to_rgba32(r, g, b)

        local index = palette[rgb]
        if not index then -- No exact match found in the palette, find the closest one.
          index = match_color_in_palette(r, g, b, palette)
          palette[rgb] = index -- Cache it for next time.
        end

        writer:write(string.pack("B", index))
        return true
    end)

  return result
end

local function write_image(path, flags, image, palette)
  local writer = io.open(path, "wb")
  if not writer then
    print(string.format("*** can't create file `%s`", path))
    return false
  end

  emit_header(writer, flags, image, palette)
  emit_data(writer, flags, image, palette)

  writer:close()

  return true
end

-- https://lospec.com/palette-list
local function load_and_parse_palette(path)
  local reader = io.open(path, "rb")
  if not reader then
    print(string.format("*** can't open palette file `%s`", path))
    return nil, 0
  end

  local palette = {}
  local palette_length = 0
  while true do
    local line = reader:read("*l")
    if not line then
      break
    end
    local rgb = tonumber(line, 16)

    if not palette[rgb] then
      if palette_length == 256 then
        print("*** too many colors in the palette (max 256)")
        reader:close()
        return nil, 0
      end

      print(string.format("new palette entry found: %08x", rgb))
      palette[rgb] = palette_length -- Store the palette index for this color.
      palette_length = palette_length + 1
    end
  end

  reader:close()

  return palette, palette_length
end

local function main(arg)
  -- https://argparse.readthedocs.io/en/stable/options.html#flags
  local parser = argparse()
    :name("imagen")
    :description("Image generator.")
  parser:argument("input")
    :description("Path of the input image to be converted.")
    :args(1)
  parser:option("-o --output")
    :description("Name of the the generated image.")
    :default("aout.img")
    :count(1)
    :args(1)
  parser:option("-c --colors")
    :description("Sets the size of the palette.")
    :default("256")
    :count(1)
    :args(1)
  parser:option("-p --palette")
    :description("Path of the palette file.")
--    :count(1)
    :args('?')
  parser:flag("-q --quiet")
    :description("Enables quiet output during image conversion.")
  parser:flag("-d --detailed")
    :description("Enables detailed output during image conversion.")
  local args = parser:parse(arg)

  for k, v in pairs(args) do
    print(string.format("arg[%s] = %s", k, tostring(v)))
  end

  local flags = {}
  for _, flag in ipairs({ "quiet", "detailed" }) do
    flags[flag] = args[flag] and true or false
  end

  if not flags.quiet then
    print("ImageN v0.1.0")
    print("=============")
  end

  local image = load_image(args.input, flags)

  if not flags.quiet then
    print(string.format("Processing image %s as %s", args.input, args.output))
  end

  local palette = args.palette
    and load_and_parse_palette(args.palette[1])
    or calculate_palette(image, args.colors)

  if not palette then
    os.exit(-1)
  end

  local success = write_image(args.output, flags, image, palette)

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
