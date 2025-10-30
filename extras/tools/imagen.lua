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
    if args.detailed then
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

local function load_image(path, flags)
  log("Loading image %s...", path)

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

-- Various metrics to calculate the "score" of a bucket.
--
-- La scelta del bucket è il punto chiave. Ecco le euristiche più efficaci
-- (ordinale dal più semplice al più sofisticato):
-- 
-- Max range — scegli il bucket con la massima estensione su R, G o B
-- (la classica scelta di Heckbert).
--   Pro: semplice.
--   Contro: ignora quanto è popolato il bucket.
-- Max peso (totalCount) — scegli il bucket con più pixel totali.
--   Pro: garantisce che split si concentri sulle aree più “importanti”.
--   Contro: potrebbe preferire bucket già stretti (poco range).
-- Max (range * totalCount) — combinazione semplice molto efficace: dà priorità
-- a bucket larghi e popolosi.
--   Spesso è la scelta pratica migliore per immagini reali.
-- Max weighted variance (o volume) — calcola la varianza pesata (somma delle
-- varianze su canali, o il volume del box) e scegli il bucket con la massima
-- varianza.
--   Più costoso ma più accurato; preferito se vuoi qualità massima.
-- Max perceptual measure — esegui i calcoli in spazio Lab e usa il volume/variance in Lab per scegliere (migliore corrispondenza percettiva).
--   Richiede conversione colore.
-- 
-- Nota: sempre controlla canSplit: non puoi splittare bucket che contengono
-- un solo colore distinto (o che non possono essere divisi in due sottoinsiemi
-- non-vuoti).
local function score_max_range(bucket)
  return bucket.info.range
end

local function score_max_weight(bucket)
  return bucket.info.count
end

local function score_max_weighted_range(bucket)
  return bucket.info.range * bucket.info.count
end

local function score_max_weighted_variance(bucket)
  return bucket.info.var
end

-- Process a bucket to compute its color range and total count.
local function process_bucket(bucket)
  local min_r, min_g, min_b = 255, 255, 255
  local max_r, max_g, max_b = 0, 0, 0
  local mu_r, mu_g, mu_b = 0, 0, 0
  local count = 0
  for _, color in ipairs(bucket) do
    if color.r < min_r then min_r = color.r end
    if color.g < min_g then min_g = color.g end
    if color.b < min_b then min_b = color.b end
    if color.r > max_r then max_r = color.r end
    if color.g > max_g then max_g = color.g end
    if color.b > max_b then max_b = color.b end
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

  local range_r = max_r - min_r
  local range_g = max_g - min_g
  local range_b = max_b - min_b

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
    range = math.max(range_r, math.max(range_g, range_b)),
    var_r = var_r,
    var_g = var_g,
    var_b = var_b,
    var = math.max(var_r, math.max(var_g, var_b)),
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
    local score = info.range * info.count
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
  log_debug("Color ranges: R=%d, G=%d, B=%d", info.range_r, info.range_g, info.range_b)
  log_debug("Total count: %d", info.count)

  -- Sort the bucket by the selected channel
  if info.range_r >= info.range_g and info.range_r >= info.range_b then
    log_debug("Sorting by R")
    table.sort(bucket, function(a, b) return a.r < b.r end)
  elseif info.range_g >= info.range_r and info.range_g >= info.range_b then
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
    log_debug("  color %08x with count %d",
      to_rgba32(color.r, color.g, color.b), color.count)
  end

  log_debug("Finding pivot point...")
  local cumulative_count = 0
  local pivot_index = 1
  local half_count = info.count / 2 -- Float division, we are OK with that.
  for index, color in ipairs(bucket) do
    cumulative_count = cumulative_count + color.count
    log_debug("  color %08x with count %d brings running count to %d",
      to_rgba32(color.r, color.g, color.b), color.count, cumulative_count)
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
        log_debug("  -> color %08x goes to bucket A", to_rgba32(color.r, color.g, color.b))
        table.insert(bucket_a, color)
      else
        log_debug("  -> color %08x goes to bucket B", to_rgba32(color.r, color.g, color.b))
        table.insert(bucket_b, color)
      end
    end
  else
    for index, color in ipairs(bucket) do
      if index <= pivot_index then
        log_debug("  -> color %08x goes to bucket A", to_rgba32(color.r, color.g, color.b))
        table.insert(bucket_a, color)
      else
        log_debug("  -> color %08x goes to bucket B", to_rgba32(color.r, color.g, color.b))
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

  log_debug("Averaging %d colors", count)
  log_debug("Total sums: R=%d, G=%d, B=%d", r_sum, g_sum, b_sum)
  local r = math.tointeger(math.floor(r_sum / count + 0.5))
  local g = math.tointeger(math.floor(g_sum / count + 0.5))
  local b = math.tointeger(math.floor(b_sum / count + 0.5))
  log_debug("Average color: R=%d, G=%d, B=%d", r, g, b)

  return to_rgba32(r, g, b)
end

local function calculate_palette(image, colors)
  local bucket = compute_histogram(image)
  if not bucket then
    return nil
  end
  local buckets = { bucket }
  log_debug("Initial bucket with %d colors", #bucket)

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

    local rgb = average_bucket_color(bucket)
    table.insert(palette, rgb)
    log_debug("new palette entry added: %08x", rgb)
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
    log("*** can't create file `%s`", path)
    return false
  end

  emit_header(writer, flags, image, palette)
  emit_data(writer, flags, image, palette)

  writer:close()

  return true
end

-- Load and parse a palette file in the simple hexadecimal format used by
-- https://lospec.com/palette-list
local function load_and_parse_palette(path)
  local reader = io.open(path, "rb")
  if not reader then
    log("*** can't open palette file `%s`", path)
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
      log("*** too many colors in the palette (max 256)")
      reader:close()
      return nil
    end

    log("new palette entry found: %08x", rgb)
    table.insert(palette, rgb)
  end

  reader:close()

  return palette
end

local function convert_command(args, flags)
  local image = load_image(args.input, flags)

  log(string.format("Processing image %s as %s", args.input, args.output))

  local palette = args.palette
    and load_and_parse_palette(args.palette)
    or calculate_palette(image, args.colors or 256)

  if not palette then
    log("*** failed to calculate palette")
    return false
  end

  for index, rgb in ipairs(palette) do
    log("palette[%3d] = %08x", index - 1, rgb)
  end

  local success = write_image(args.output, flags, image, palette)

  log(success and "Done!" or "Failed!")

  return success
end

local function main(arg)
  -- https://argparse.readthedocs.io/en/stable/options.html#flags
  local parser = argparse()
    :name("imagen")
    :description("Image generator.")
  parser:argument("command")
    :choices({'convert', 'extract'})
    :description("Command to be executed (only `convert` is supported).")
    :args(1)
  parser:argument("input")
    :description("Path of the input image to be converted.")
    :args(1)
  parser:option("-o --output")
    :description("Name of the the generated image.")
    :default("aout.img")
    :args('?')
    :count(1)
  parser:option("-c --colors")
    :description("Sets the size of the palette.")
    :default("256")
    :args('?')
    :convert(math.tointeger)
    :count(1)
  parser:option("-p --palette")
    :description("Path of the palette file.")
    :default("")
    :args('?')
    :count(1)
  parser:flag("-q --quiet")
    :description("Enables quiet output during image conversion.")
  parser:flag("-d --detailed")
    :description("Enables detailed output during image conversion.")
  local args = parser:parse(arg)

  --for k, v in pairs(args) do
  --  print(string.format("arg[%s] = %s", k, tostring(v)))
  --end

  log_init(args)

  log("ImageN v0.2.0")
  log("=============")

  local success = false
  if args.command == "convert" then
    success = convert_command(args, flags)
  end

  os.exit(not success and -1 or 0)
end

main(arg)
