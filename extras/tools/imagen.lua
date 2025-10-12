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
local function calculate_palette(image)
  local palette = {}
  local palette_length = 0
  local result = for_each_pixel(image,
    function(x, y, r, g, b, a)
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

        return true
    end)

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
    :description("Sets the size of the palette.")
--    :count(1)
    :args('?')
  parser:flag("-q --quiet")
    :description("Enables quiet output during image conversion.")
  parser:flag("-d --detailed")
    :description("Enables detailed output during image conversion.")
  local args = parser:parse(arg)

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
    or calculate_palette(image)

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
