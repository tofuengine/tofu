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

local Class = require("tofu.core.class")
local Canvas = require("tofu.graphics.canvas")
local Display = require("tofu.graphics.display")
local Program = require("tofu.graphics.program")

local config = require("config")

local RUMBLE_LENGTH <const> = config.scene.ground.rumble_length

local SKY <const> = {
  { 0, 0x00, 0xBE, 0xDA },
  { 0, 0x8F, 0xB6, 0xBD },
  { 0, 0xC2, 0xA3, 0xA0 }
}

local Background = Class.define()

function Background:__ctor(camera, index)
  self.skyline = Canvas.new("assets/skyline.png", index)
  self.camera = camera
  self.index = index
  self.program = Program.new()
end

-- Rebuild sky and ground (copperlist) program.
local function _compile_program(program, camera, index)
  local x <const> = camera.x
  local far <const> = camera.far + camera.z
  local near <const> = camera.near + camera.z

  local _, _, _, _, sy0 = camera:project(x, 0.0, far) -- Straight forward, on ground level.
  local y0 = math.tointeger(sy0 + 0.5)

--  SKY[1][1] = 0
  SKY[2][1] = y0 * 0.75
  SKY[3][1] = y0 - 1

  program:clear()
  program:gradient(index, SKY)

  local toggle = (far // RUMBLE_LENGTH) % 2 == 0 -- Find the initial color.
  program:wait(0, y0)
  program:shift(index, toggle and 28 or 42)
  program:shift(index + 1, toggle and 42 or 28)

  local less_far <const> = far - (far % RUMBLE_LENGTH) -- Skip to the start of the new rumble.

  for z = less_far, near, -RUMBLE_LENGTH do
    local _, _, _, _, sy = camera:project(x, 0.0, z) -- Straight forward, on ground level.
    toggle = not toggle
    program:wait(0, math.tointeger(sy + 0.5))
    program:shift(index, toggle and 28 or 42)
    program:shift(index + 1, toggle and 42 or 28)
  end
end

function Background:update(_)
  local program = self.program
  _compile_program(program, self.camera, self.index)
  Display.program(program)
end

function Background:render(canvas)
  local camera <const> = self.camera
  local x <const> = camera.x
  local far <const> = camera.far + camera.z
  local near <const> = camera.near + camera.z

  local _, _, _, _, sy = camera:project(x, 0.0, far) -- Straight forward, on ground level.
  local y = math.tointeger(sy + 0.5)

  -- Render the skyline.
  local width, _ = canvas:size()
  local w, h = self.skyline:size()
  h = h - (15 - camera.y // 16)
  local wy = y - h
  local offset_x <const> = camera.x // 4
  for wx = 0, width, w do
    canvas:tile(wx, wy, self.skyline, 0, 0, w, h, offset_x, 0)
  end

  -- Render the "road"
  local _, _, _, sx0, sy0 = camera:project(-100, 0.0, far)
  local _, _, _, sx1, sy1 = camera:project( 100, 0.0, far)
  local _, _, _, sx2, sy2 = camera:project(-100, 0.0, near)
  local _, _, _, sx3, sy3 = camera:project( 100, 0.0, near)

  canvas:triangle("fill", math.tointeger(sx0 + 0.5), math.tointeger(sy0 + 0.5),
      math.tointeger(sx2 + 0.5), math.tointeger(sy2 + 0.5),
      math.tointeger(sx1 + 0.5), math.tointeger(sy1 + 0.5), self.index + 1)
  canvas:triangle("fill", math.tointeger(sx1 + 0.5), math.tointeger(sy1 + 0.5),
      math.tointeger(sx2 + 0.5), math.tointeger(sy2 + 0.5),
      math.tointeger(sx3 + 0.5), math.tointeger(sy3 + 0.5)  , self.index + 1)
--    print(">>", sx0, sy0)
--    print("  ", sx1, sy1)
--    print("  ", sx2, sy2)
--    print("  ", sx3, sy3)
end

return Background
