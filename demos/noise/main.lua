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

local Class = require("tofu.core.class")
local System = require("tofu.core.system")
local Controller = require("tofu.input.controller")
local Noise = require("tofu.generators.noise")
local Canvas = require("tofu.graphics.canvas")
local Display = require("tofu.graphics.display")
local Font = require("tofu.graphics.font")
local Palette = require("tofu.graphics.palette")
local Grid2D = require("tofu.util.grid2d")

local PALETTE <const> = Palette.new(256)
local FONT <const> = Font.default()
local CANVAS <const> = Canvas.default()
local WIDTH <const>, HEIGHT <const> = CANVAS:image():size()
local CONTROLLER <const> = Controller.default()

local NOISES <const> = {
    "perlin",
    "simplex",
    "cellular"
  }

local Main = Class.define()

function Main:__ctor()
  self.current = 1

  self.noise = Noise.new(NOISES[self.current])
  self.grid = Grid2D.new(WIDTH, HEIGHT)

  self.min = 0
  self.max = 1
  self.frequency = 5
end

function Main:init()
  Display.palette(PALETTE)
end

function Main:deinit()
end

function Main:handle_input()
  if CONTROLLER:is_pressed("right") then
    self.current = (self.current % #NOISES) + 1
    self.noise:type(NOISES[self.current])
  elseif CONTROLLER:is_pressed("left") then
    self.current = ((self.current + (#NOISES - 2)) % #NOISES) + 1
    self.noise:type(NOISES[self.current])
  end
  if CONTROLLER:is_pressed("up") then
    self.frequency = self.frequency + 1
  elseif CONTROLLER:is_pressed("down") then
    self.frequency = self.frequency - 1
  end
end

-- https://www.redblobgames.com/maps/terrain-from-noise/
function Main:update(_)
  self:handle_input()

  local time <const> = System.time() * 0.1
  local nz = time

  local octaves = 3
  local grid = self.grid
  local noise = self.noise
  local min, max = math.huge, -math.huge
  for y = 0, HEIGHT - 1 do
    local ny = y / HEIGHT + 0.5 -- Scale into `[0, 1]` to make frequency work.
    for x = 0, WIDTH - 1 do
      local nx = x / WIDTH + 0.5 -- Ditto.

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

function Main:render(canvas, _)
  canvas:clear(0)

  local scale = 255 / (self.max - self.min)
  canvas:scan(function(x, y, _)
      local v = self.grid:peek(x, y)
      return math.tointeger((v - self.min) * scale)
    end)

  canvas:write(0, 0, FONT, string.format("FPS: %d (%s, %d)", System.fps(), NOISES[self.current], self.frequency))
end

return Main
