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

Copyright (c) 2019-2026 Marco Lizza

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

-- The image format use is the following:
--   [0-7]     : (8 bytes) Signature `TOFUIMG!`
--   [8-9]     : (2 bytes) Width (little-endian)
--   [10-11]   : (2 bytes) Height (little-endian)
--   [12-13]   : (2 bytes) Palette length, up to 256 entries (little-endian)
--   [14-...]  : (palette length * 3 bytes) Palette data (RGB triplets)
--   [...-end] : (width * height bytes) Pixel data (each byte is an 8-bit index in the palette)

local argparse = require("argparse")
local lfs = require("lfs")
local vips = require("vips")

-- -----------------------------------------------------------------------------
-- HELPER FUNCTIONS ------------------------------------------------------------
-- -----------------------------------------------------------------------------

-- Initialize the logging functions based on the current command-line flags.
-- By default, logging is enabled. If the `quiet` flag is set, logging is
-- disabled. If the `verbose` flag is set, verbose logging is enabled.
local function log_init(args)
  if args.quiet then
    log = function(...)
      -- No-op
    end
    log_debug = function(...)
      -- No-op
    end
  else
    log = function(...)
      print(string.format(...))
    end
    if args.verbose then
      log_debug = function(...)
        print(string.format(...))
      end
    else
      log_debug = function(...)
        -- No-op
      end
    end
  end
end

local function load_image(path)
  log("Loading image `%s`", path)

  local image = vips.Image.new_from_file(path, { access = "random" })
  if image:bands() == 3 then
      image = image:bandjoin({255})
  end
  image = image:cast("uchar")

  log("Image size: %dx%d", image:width(), image:height())
  log("Image bands: %d", image:bands())

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

-- -----------------------------------------------------------------------------
-- COLOR QUANTIZATION ----------------------------------------------------------
-- -----------------------------------------------------------------------------

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

  local colors = {}
  for rgb, count in pairs(histogram) do
    log_debug("Color %08x: %d occurrences", rgb, count)
    local r, g, b = from_rgba32(rgb)
    table.insert(colors, { r = r, g = g, b = b, count = count })
  end
  return colors
end

-- Process a bucket to compute its color maximum weighted variance. This
-- is more computationally expensive than just computing the range, but it gives
-- better results. Specifically, it avoids splitting buckets where the range is
-- determined by outlier colors with very low occurrence counts. The variance
-- is a more stable metric in this sense, as it takes into account of the
-- internal distribution of colors in the bucket (and not just the extreme
-- values).
local function process_bucket(bucket)
  local mu_r, mu_g, mu_b = 0, 0, 0
  local count = 0
  for _, color in ipairs(bucket) do
    mu_r = mu_r + color.r * color.count
    mu_g = mu_g + color.g * color.count
    mu_b = mu_b + color.b * color.count
    count = count + color.count
  end
  mu_r = mu_r / count
  mu_g = mu_g / count
  mu_b = mu_b / count

  local var_r, var_g, var_b = 0, 0, 0
  for _, color in ipairs(bucket) do
    local dr = color.r - mu_r
    local dg = color.g - mu_g
    local db = color.b - mu_b
    var_r = var_r + (dr * dr) * color.count
    var_g = var_g + (dg * dg) * color.count
    var_b = var_b + (db * db) * color.count
  end
  var_r = var_r / count
  var_g = var_g / count
  var_b = var_b / count

  return {
    var_r = var_r,
    var_g = var_g,
    var_b = var_b,
    variance = math.max(var_r, math.max(var_g, var_b)),
    count = count
  }
end

-- A bucket can be split if it contains at least one. Since each color in a
-- bucket is unque (by construction), we don't need to check for range
-- variations. Colors are always different each other.
local function can_split_bucket(bucket)
  return #bucket > 1
end

-- We scan the buckets and pick the one with the largest color range, weighted
-- by the number of colors it contains.
local function pick_best_bucket(buckets)
  local best_index = nil
  local best_score = -1

  for index, bucket in ipairs(buckets) do
    -- We don´t really need to check for splittability for every bucket here,
    -- as we could just pick the best one and check for splittability later. This
    -- way we can avoid unnecessary processing, and we also keep the code
    -- more generic. Checking for splittability on return (to halt the process)
    -- would work only if we pick the "best bucket" considering it's range and
    -- so actually skipping "null" buckets.
    if not can_split_bucket(bucket) then
      log_debug("Bucket %d can't be split anymore", index)
      goto continue
    end

    local info = process_bucket(bucket)
    local score = info.variance
    if score > best_score then
      best_score = score
      best_index = index
    end

    ::continue::
  end

  return best_index
end

local function split_bucket(bucket)
  -- Determine the channel with the largest range
  local info = process_bucket(bucket)
  log_debug("Color variances: R=%.2f, G=%.2f, B=%.2f", info.var_r, info.var_g, info.var_b)
  log_debug("Total count: %d", info.count)

  -- Sort the bucket by the selected channel
  if info.variance == info.var_r then
    log_debug("Sorting by R")
    table.sort(bucket, function(a, b) return a.r < b.r end)
  elseif info.variance == info.var_g then
    log_debug("Sorting by G")
    table.sort(bucket, function(a, b) return a.g < b.g end)
  else
    log_debug("Sorting by B")
    table.sort(bucket, function(a, b) return a.b < b.b end)
  end

  -- Find the pivot point, that is the index of the element that cause the
  -- cumulative count to reach half of the total count.
  --
  -- pivot = first index i such that cumulative[i] >= total/2
  for index, color in ipairs(bucket) do
    log_debug("  color %02x%02x%02x with count %d",
      color.r, color.g, color.b, color.count)
  end

  log_debug("Finding pivot point...")
  local cumulative_count = 0
  local pivot_index = 1
  local half_count = info.count / 2 -- Float division, we are OK with that.
  for index, color in ipairs(bucket) do
    cumulative_count = cumulative_count + color.count
    log_debug("  color %02x%02x%02x with count %d brings running count to %d",
      color.r, color.g, color.b, color.count, cumulative_count)
    if cumulative_count >= half_count then
      pivot_index = index
      log_debug("  -> pivot found at index %d", pivot_index)
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
    log_debug("Dominant color detected at pivot")
    for index, color in ipairs(bucket) do
      if index == pivot_index then
        log_debug("  -> color %02x%02x%02x goes to bucket A", color.r, color.g, color.b)
        table.insert(bucket_a, color)
      else
        log_debug("  -> color %02x%02x%02x goes to bucket B", color.r, color.g, color.b)
        table.insert(bucket_b, color)
      end
    end
  else
    for index, color in ipairs(bucket) do
      if index <= pivot_index then
        log_debug("  -> color %02x%02x%02x goes to bucket A", color.r, color.g, color.b)
        table.insert(bucket_a, color)
      else
        log_debug("  -> color %02x%02x%02x goes to bucket B", color.r, color.g, color.b)
        table.insert(bucket_b, color)
      end
    end
  end

  return bucket_a, bucket_b
end

-- Compute the average color of a bucket (which contains multiple colors), as
-- the weighter mean.
local function average_bucket_color(bucket)
  local r_sum, g_sum, b_sum = 0, 0, 0
  local count = 0
  for _, color in ipairs(bucket) do
    r_sum = r_sum + color.r * color.count
    g_sum = g_sum + color.g * color.count
    b_sum = b_sum + color.b * color.count
    count = count + color.count
  end

  log_debug("Averaging %d colors", count)
  log_debug("Total sums: R=%d, G=%d, B=%d", r_sum, g_sum, b_sum)
  local r = math.tointeger(math.floor(r_sum / count + 0.5))
  local g = math.tointeger(math.floor(g_sum / count + 0.5))
  local b = math.tointeger(math.floor(b_sum / count + 0.5))
  log_debug("Average color: R=%d, G=%d, B=%d", r, g, b)

  return { r = r, g = g, b = b }
end

-- Calculate a palette of the given size from the image using the median-cut
-- algorithm.
local function calculate_palette(image, colors)
  local bucket = compute_histogram(image)
  if not bucket then
    return nil
  end
  local buckets = { bucket }
  log_debug("Initial bucket with %d colors", #bucket)

  -- Keep splitting until we reach the desired number of colors on no more splittable buckets.
  while #buckets < colors do
    local index = pick_best_bucket(buckets)
    if not index then
      log_debug("*** no more splittable buckets available")
      break
    end

    local bucket = buckets[index]
    log_debug("Splitting bucket %d with %d colors", index, #bucket)

    local bucket_a, bucket_b = split_bucket(bucket)
    assert(#bucket_a > 0 and #bucket_b > 0, "bucket split resulted in an empty bucket, aborting")
    table.remove(buckets, index)
    table.insert(buckets, bucket_a)
    table.insert(buckets, bucket_b)
  end

  local palette = {}
  for _, bucket in ipairs(buckets) do
    log_debug("Final bucket with %d colors", #bucket)

    local color = average_bucket_color(bucket)
    table.insert(palette, color)
    log_debug("new palette entry added: %02x%02x%02x", color.r, color.g, color.b)
  end

  return palette
end

-- Find the best matching color in the palette for the given RGB color.
-- The best matching color is the one with the smallest Euclidean distance
-- in the RGB color space.
-- If no color is found (which should not happen), returns palette index 0.
local function match_color_in_palette(r, g, b, palette, exclude)
  local best_index = 0
  local best_distance = math.huge
  for index, color in ipairs(palette) do
    if exclude and index - 1 == exclude then -- Transparent color can be `nil` if disabled
      log_debug("  skipping excluded palette index %d", index - 1)
      goto continue
    end
    local dr = r - color.r
    local dg = g - color.g
    local db = b - color.b
    local distance = (dr * dr) + (dg * dg) + (db * db)
    if distance < best_distance then
      best_distance = distance
      best_index = index - 1 -- Palette index is zero-based.
    end
    ::continue::
  end
  return best_index
end

-- -----------------------------------------------------------------------------
-- I/O FUNCTIONS ---------------------------------------------------------------
-- -----------------------------------------------------------------------------

local function emit_header(writer, header)
  writer:write(string.pack("c8", "TOFUIMG!"))
  writer:write(string.pack("<I2", header.width))
  writer:write(string.pack("<I2", header.height))
  writer:write(string.pack("<I2", #header.palette))
  for _, color in ipairs(header.palette) do
    writer:write(string.pack("BBB", color.r, color.g, color.b))
  end
  for index = #header.palette + 1, 256 do
    local pixel = index - 1
    writer:write(string.pack("BBB", pixel, pixel, pixel)) -- Pad the palette to 256 entries.
  end
end

local function emit_data(writer, image, palette, transparent)
  local cache = {} -- Memoization cache for color lookups.

  for_each_pixel(image,
    function(x, y, r, g, b, a)
        if transparent and a < 255 then
          writer:write(string.pack("B", transparent))
          return
        end

        local rgb = to_rgba32(r, g, b)

        local index = cache[rgb]
        if not index then -- Not cached yet. Find (the best matching) color in palette.
          index = match_color_in_palette(r, g, b, palette, transparent)
          cache[rgb] = index -- Cache it for later use.
        end

        writer:write(string.pack("B", index))
    end)
end

local function write_image(path, image, palette, transparent)
  local writer = io.open(path, "wb")
  if not writer then
    log("*** can't create file `%s`", path)
    return false
  end

  local header = {
    width = image:width(),
    height = image:height(),
    palette = palette
  }

  emit_header(writer, header)
  emit_data(writer, image, palette, transparent)

  writer:close()

  return true
end

-- The palette is a Lua file that return, as a module, a table of RGB colors.
local function load_and_parse_palette(path)
  local palette_module = dofile(path)
  if not palette_module then
    log("*** can't load palette module `%s`", path)
    return nil
  end

  local palette = {}
  for index, color in ipairs(palette_module) do
    if #palette == 256 then
      log("*** too many colors in the palette (max 256)")
      return nil
    end

    log_debug("new palette entry found: %02x%02x%02x", color[1], color[2], color[3])
    table.insert(palette, { r = color[1], g = color[2], b = color[3] })
  end

  return palette
end

local function read_header(reader)
  local signature, width, height, palette_size = string.unpack("c8<I2<I2<I2", reader:read(14))

  if signature ~= "TOFUIMG!" then
    log("*** invalid image signature: expected `TOFUIMG!`, got `%s`", signature)
    return nil
  end

  local palette = {}
  for i = 1, palette_size do
    local r, g, b = string.unpack("BBB", reader:read(3))
    table.insert(palette, { r = r, g = g, b = b })
  end

  return {
    width = width,
    height = height,
    palette = palette
  }
end

-- -----------------------------------------------------------------------------
-- COMMANDS --------------------------------------------------------------------
-- -----------------------------------------------------------------------------

local function convert_command(input, palette, colors, sort, transparent)
  local image = load_image(input)

  log("Converting image %s", input)

  local palette = #palette > 0
    and load_and_parse_palette(palette)
    or calculate_palette(image, colors or 256)

  if not palette then
    log("*** failed to obtain the palette")
    return false
  end

  if sort then
    log("sorting palette...")
    table.sort(palette, function(a, b)
        return a.r < b.r or
                (a.r == b.r and a.g < b.g) or
                (a.r == b.r and a.g == b.g and a.b < b.b)
      end)
  end

  log("Palette size: %d colors", #palette)
  for index, color in ipairs(palette) do
    log("  [%3d] = %02x%02x%02x", index - 1, color.r, color.g, color.b)
  end

  -- The output file name is the same of the input, but with `.img` extension.
  local output = input:gsub("%.%w+$", "") .. ".img"
  log("Writing output image `%s`", output)
  local success = write_image(output, image, palette, transparent)

  log(success and "Done!" or "Failed!")

  return success
end

-- Reads and displays the header of an image file. The size and the palette
-- colors are printed. This is useful to check the actual dimensions of an
-- image and the palette bound to it.
local function command_inspect(input)
  log("Inspecting image `%s`", input)

  local reader = io.open(input, "rb")
  if not reader then
    log("*** can't open file `%s`", input)
    return false
  end

  local header = read_header(reader, image)
  if not header then
    log("*** failed to read image header")
    return false
  end

  log("Image size: %dx%d", header.width, header.height)
  log("Palette size: %d colors", #header.palette)
  for index, color in ipairs(header.palette) do
    log("  [%3d] = %02x%02x%02x", index - 1, color.r, color.g, color.b)
  end

  reader:close()

  return true
end

local function is_directory(path)
  return path:sub(-1) == "/" or lfs.attributes(path, "mode") == "directory"
end

local function expand_path(path, filter, paths)
  if is_directory(path) then
    for entry in lfs.dir(path) do
      if entry ~= "." and entry ~= ".." then
        expand_path(path .. "/" .. entry, filter, paths)
      end
    end
  elseif path:match(filter) then
    table.insert(paths, path)
  end
end

local function main(arg)
  -- https://argparse.readthedocs.io/en/stable/options.html#flags
  local parser = argparse()
    :name("imagen")
    :description("Image generator.")
  parser:argument("command")
    :choices({'convert', 'inspect'})
    :description("Command to be executed.")
    :args(1)
  parser:argument("input")
    :description("Path of the input image to be converted.")
    :args(1)
  parser:option("-c --colors")
    :description("Sets the size of the palette.")
    :default("256")
    :args(1)
    :count(1)
    :convert(math.tointeger)
  parser:option("-p --palette")
    :description("Path of the palette file.")
    :default("")
    :args(1)
    :count(1)
  parser:option("-t --transparent")
    :description("Index of the transparent index in the palette.")
    :default("0")
    :args(1)
    :count(1)
    :convert(math.tointeger)
  parser:flag("-o --opaque")
    :description("Don't consider the transparent index.")
  parser:flag("-s --sort")
    :description("Sort the palette entries before converting the image.")
  parser:flag("-q --quiet")
    :description("Suppress any output during image conversion.")
  parser:flag("-v --verbose")
    :description("Enables verbose output during image conversion.")
  local args = parser:parse(arg)

  --for k, v in pairs(args) do
  --  print(string.format("arg[%s] = %s", k, tostring(v)))
  --end

  log_init(args)

  log("ImageN v0.3.0")
  log("=============")

  local paths = {}
  expand_path(args.input, "%.png$", paths)

  local success = false
  for _, path in ipairs(paths) do
    if args.command == "convert" then
      success = convert_command(path, args.palette, args.colors, args.sort, not args.opaque and args.transparent or nil)
    elseif args.command == "inspect" then
      success = command_inspect(path)
    end
    if not success then
      break
    end
  end

  os.exit(not success and -1 or 0)
end

main(arg)
