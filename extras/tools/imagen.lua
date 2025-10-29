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

          callback(x, y, r, g, b, a)

          offset = offset + 4
      end
  end
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

-- Iterate over each pixel in the image and compute the histogram of colors.
-- The histogram is a table mapping from color (as 32-bit RGB integer) to
-- occurrence count.
local function compute_histogram(image)
  local histogram = {}

  for_each_pixel(image,
    function(x, y, r, g, b, a)
        local rgb = to_rgba32(r, g, b)
        histogram[rgb] = (histogram[rgb] or 0) + 1
    end)

  local buckets = {}
  for rgb, count in pairs(histogram) do
    print(string.format("Color %08x: %d occurrences", rgb, count))
    local r, g, b = from_rgba32(rgb)
    table.insert(buckets, { r = r, g = g, b = b, count = count })
  end
  return buckets
end

-- Process a bucket to compute its color range and total count.
local function process_bucket(bucket)
  local min_r, min_g, min_b = 255, 255, 255
  local max_r, max_g, max_b = 0, 0, 0
  local count = 0
  for _, color in ipairs(bucket) do
    if color.r < min_r then min_r = color.r end
    if color.g < min_g then min_g = color.g end
    if color.b < min_b then min_b = color.b end
    if color.r > max_r then max_r = color.r end
    if color.g > max_g then max_g = color.g end
    if color.b > max_b then max_b = color.b end
    count = count + color.count
  end
  local range_r = max_r - min_r
  local range_g = max_g - min_g
  local range_b = max_b - min_b
  local range = math.max(range_r, math.max(range_g, range_b))

  return {
    min_r = min_r,
    min_g = min_g,
    min_b = min_b,
    max_r = max_r,
    max_g = max_g,
    max_b = max_b,
    range_r = range_r,
    range_g = range_g,
    range_b = range_b,
    range = range,
    count = count
  }
end

-- We scan the buckets and pick the one with the largest color range, weighted
-- by the number of colors it contains.
local function pick_best_bucket(buckets)
  local best_index = 1
  local best_score = -1

  for index, bucket in ipairs(buckets) do
    local info = process_bucket(bucket)
    local score = info.range * info.count
    if score > best_score then
      best_score = score
      best_index = index
    end
  end

  return best_index
end

-- A bucket can be split if and only if it contains at least one color.
local function can_split_bucket(bucket)
  return #bucket > 1
end

local function split_bucket(bucket)
  -- Determine the channel with the largest range
  local info = process_bucket(bucket)
  print(string.format("Color ranges: R=%d, G=%d, B=%d", info.range_r, info.range_g, info.range_b))
  print(string.format("Total count: %d", info.count))

  -- Sort the bucket by the selected channel
  if info.range_r >= info.range_g and info.range_r >= info.range_b then
    print("Sorting by R")
    table.sort(bucket, function(a, b) return a.r < b.r end)
  elseif info.range_g >= info.range_r and info.range_g >= info.range_b then
    print("Sorting by G")
    table.sort(bucket, function(a, b) return a.g < b.g end)
  else
    print("Sorting by B")
    table.sort(bucket, function(a, b) return a.b < b.b end)
  end

  -- Find the pivot point, that is the index of the element that cause the
  -- cumulative count to reach half of the total count.
  --
  -- pivot = first index i such that cumulative[i] >= total/2
  for index, color in ipairs(bucket) do
    print(string.format("  color %08x with count %d",
      to_rgba32(color.r, color.g, color.b), color.count))
  end

  print("Finding pivot point...")
  local cumulative_count = 0
  local pivot_index = 1
  local half_count = info.count / 2 -- Float division, we are OK with that.
  for index, color in ipairs(bucket) do
    cumulative_count = cumulative_count + color.count
    print(string.format("  color %08x with count %d brings running count to %d",
      to_rgba32(color.r, color.g, color.b), color.count, cumulative_count))
    if cumulative_count >= half_count then
      pivot_index = index
      print(string.format("  -> pivot found at index %d", pivot_index))
      break
    end
  end

  -- Split the bucket into two new buckets
  local bucket_a = {}
  local bucket_b = {}

  -- Dominant color check: the pivot color surpasses half of the total count.
  -- This will ensure that we are not creating empty buckets when filling the
  -- buckets with a running cumulative count (i.e. if the pivot is at the
  -- beginning or the end of the bucket). Anyway, in the other cases we just
  -- we are essentially optimizing the split.
  if bucket[pivot_index].count >= half_count then -- Dominant color detected!
    print("Dominant color detected at pivot")
    for index, color in ipairs(bucket) do
      if index == pivot_index then
        print(string.format("  -> color %08x goes to bucket A", to_rgba32(color.r, color.g, color.b)))
        table.insert(bucket_a, color)
      else
        print(string.format("  -> color %08x goes to bucket B", to_rgba32(color.r, color.g, color.b)))
        table.insert(bucket_b, color)
      end
    end
  else
    for index, color in ipairs(bucket) do
      if index <= pivot_index then
        print(string.format("  -> color %08x goes to bucket A", to_rgba32(color.r, color.g, color.b)))
        table.insert(bucket_a, color)
      else
        print(string.format("  -> color %08x goes to bucket B", to_rgba32(color.r, color.g, color.b)))
        table.insert(bucket_b, color)
      end
    end
  end

  return bucket_a, bucket_b
end

local function average_bucket_color(bucket)
  local r_sum, g_sum, b_sum = 0, 0, 0
  local count = 0
  for _, color in ipairs(bucket) do
    r_sum = r_sum + color.r * color.count
    g_sum = g_sum + color.g * color.count
    b_sum = b_sum + color.b * color.count
    count = count + color.count
  end

  print(string.format("Averaging %d colors", count))
  print(string.format("Total sums: R=%d, G=%d, B=%d", r_sum, g_sum, b_sum))
  local r = math.tointeger(math.floor(r_sum / count + 0.5))
  local g = math.tointeger(math.floor(g_sum / count + 0.5))
  local b = math.tointeger(math.floor(b_sum / count + 0.5))
  print(string.format("Average color: R=%d, G=%d, B=%d", r, g, b))

  return to_rgba32(r, g, b)
end

local function calculate_palette(image, colors)
  local bucket = compute_histogram(image)
  if not bucket then
    return nil
  end
  local buckets = { bucket }
  print(string.format("Initial bucket with %d colors", #bucket))

  while #buckets < colors do
    -- Pick the best bucket to be split
    local index = pick_best_bucket(buckets)
    local bucket = buckets[index]
    print(string.format("Splitting bucket %d with %d colors", index, #bucket))

    -- Check if the bucket can be split. If the selected bucket can't be
    -- split anymore, we are done.
    if not can_split_bucket(bucket) then
      print("*** no more splittable buckets available")
      break
    end

    -- Split the bucket into two new buckets
    local bucket_a, bucket_b = split_bucket(bucket)
    table.remove(buckets, index)
    if #bucket_a == 0 or #bucket_b == 0 then
      print("*** bucket split resulted in an empty bucket, aborting")
      break
    end
    table.insert(buckets, bucket_a)
    table.insert(buckets, bucket_b)
  end

  local palette = {}
  for _, bucket in ipairs(buckets) do
    print(string.format("Final bucket with %d colors", #bucket))

    local rgb = average_bucket_color(bucket)
    table.insert(palette, rgb)
    print(string.format("new palette entry added: %08x", rgb))
  end

  return palette
end

local function emit_header(writer, flags, image, palette)
  local width, height = image:width(), image:height()

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
  writer:write(string.pack("<I2", #palette))
  for _, rgb in ipairs(palette) do
    local r, g, b = from_rgba32(rgb)
    writer:write(string.pack("BBB", r, g, b))
  end
end

local function match_color_in_palette(r, g, b, palette)
  local best_index = 0
  local best_distance = math.huge
  for index, crgb in ipairs(palette) do
    local cr, cg, cb = from_rgba32(crgb)
    local dr = r - cr
    local dg = g - cg
    local db = b - cb
    local distance = (dr * dr) + (dg * dg) + (db * db)
    if distance < best_distance then
      best_distance = distance
      best_index = index - 1 -- Palette index is zero-based.
    end
  end
  return best_index
end

-- pixel data (width * height bytes, each byte is an index in the palette)
local function emit_data(writer, flags, image, palette)
  local cache = {} -- Memoization cache for color lookups.

  for_each_pixel(image,
    function(x, y, r, g, b, a)
        local rgb = to_rgba32(r, g, b)

        local index = cache[rgb]
        if not index then -- Not cached yet. Find (the best matching) color in palette.
          index = match_color_in_palette(r, g, b, palette)
          cache[rgb] = index -- Cache it for later use.
        end

        writer:write(string.pack("B", index))
    end)
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
    return nil
  end

  local palette = { }
  while true do
    local line = reader:read("*l")
    if not line then
      break
    end
    local rgb = tonumber(line, 16)

    if #palette == 256 then
      print("*** too many colors in the palette (max 256)")
      reader:close()
      return nil
    end

    print(string.format("new palette entry found: %08x", rgb))
    table.insert(palette, rgb)
  end

  reader:close()

  return palette
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
    print("ImageN v0.2.0")
    print("=============")
  end

  local image = load_image(args.input, flags)

  if not flags.quiet then
    print(string.format("Processing image %s as %s", args.input, args.output))
  end

  local palette = args.palette
    and load_and_parse_palette(args.palette[1])
    or calculate_palette(image, math.tointeger(args.colors))

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
