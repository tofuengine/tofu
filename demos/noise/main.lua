--[[
MIT License

Copyright (c) 2019-2021 Marco Lizza

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

local Class = require("tofu.core").Class
local System = require("tofu.core").System
local Input = require("tofu.events").Input
local Noise = require("tofu.generators").Noise
local Canvas = require("tofu.graphics").Canvas
local Display = require("tofu.graphics").Display
local Font = require("tofu.graphics").Font
local Palette = require("tofu.graphics").Palette
local Grid = require("tofu.util").Grid

local Main = Class.define()

local COLORS <const> = 256

local NOISES <const> = {
    "perlin",
    "simplex",
    "cellular"
  }

function Main:__ctor()
  local palette = Palette.new(COLORS)
  Display.palette(palette)

  local canvas = Canvas.default()
  local width, height = canvas:size()

  self.current = 1

  self.font = Font.default(0, COLORS - 1)
  self.noise = Noise.new(NOISES[self.current])
  self.grid = Grid.new(width, height)

  self.min = 0
  self.max = 1
  self.frequency = 5
end

function Main:process()
  if Input.is_pressed("right") then
    self.current = (self.current % #NOISES) + 1
    self.noise:type(NOISES[self.current])
  elseif Input.is_pressed("left") then
    self.current = ((self.current + (#NOISES - 2)) % #NOISES) + 1
    self.noise:type(NOISES[self.current])
  end
  if Input.is_pressed("up") then
    self.frequency = self.frequency + 1
  elseif Input.is_pressed("down") then
    self.frequency = self.frequency - 1
  end
end

-- https://www.redblobgames.com/maps/terrain-from-noise/
function Main:update(_)
  local canvas = Canvas.default()
  local width, height = canvas:size()

  local time <const> = System.time() * 0.1
  local nz = time

  local octaves = 3
  local grid = self.grid
  local noise = self.noise
  local min, max = math.huge, -math.huge
  for y = 0, height - 1 do
    local ny = y / height + 0.5 -- Scale into `[0, 1]` to make frequency work.
    for x = 0, width - 1 do
      local nx = x / width + 0.5 -- Ditto.

      local frequency = self.frequency
      local amplitude = 1.0
      local seed = frequency * amplitude + frequency + amplitude
      noise:seed(seed) -- Break octaves' correlation, should be random.

      local v = 0.0
      for _ = 1, octaves do
        noise:frequency(frequency)

        v = v + noise:generate(nx, ny, nz) * amplitude

        frequency = frequency * 2 -- Each octave, by definition, doubles the frequency...
        amplitude = amplitude * 0.5 -- ... but also halves the amplitude.
      end
      grid:poke(x, y, v)

      min = min < v and min or v
      max = max > v and max or v
    end
  end
  self.min = min
  self.max = max
  --print(">", min, max)
end

function Main:render(_)
  local canvas = Canvas.default()
  canvas:clear()

  local scale = (COLORS - 1) / (self.max - self.min)
  canvas:scan(function(x, y, _)
      local v = self.grid:peek(x, y)
      return math.tointeger((v - self.min) * scale)
    end)

  self.font:write(canvas, 0, 0, string.format("FPS: %d (%s, %d)", System.fps(), NOISES[self.current], self.frequency))
end

return Main
