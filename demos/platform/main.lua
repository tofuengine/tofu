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
local Math = require("tofu.core.math")
local System = require("tofu.core.system")
local Controller = require("tofu.input.controller")
local Bank = require("tofu.graphics.bank")
local Canvas = require("tofu.graphics.canvas")
local Display = require("tofu.graphics.display")
local Font = require("tofu.graphics.font")
local Image = require("tofu.graphics.image")
local Palette = require("tofu.graphics.palette")
local Program = require("tofu.graphics.program")
local Vector2D = require("tofu.util.vector2d")

local Animation = require("lib/animation")

local PALETTE <const> = Palette.default("nes")
local FONT <const> = Font.default()
local CANVAS <const> = Canvas.default()
local WIDTH <const>, HEIGHT <const> = CANVAS:image():size()
local CONTROLLER <const> = Controller.default()

local WATER_DISPLACEMENT = 1.5

local SNOW = false

local Main = Class.define()

local function generate_map(screens)
  local map = {}
  for i = 1, 5 do
    local row = {}
    for _ = 1, 15 * screens do
      local cell_id
      if i == 1 then
        cell_id = math.random(3, 8)
      else
        cell_id = i == 2 and 1 or 2
      end
      table.insert(row, cell_id)
    end
    table.insert(map, row)
  end
  return map
end

function Main:__ctor()
  self.atlas = Image.new(1, 1)
  self.pixies = Bank.new(self.atlas)
  self.bank = Bank.from_image("assets/sprites.img", 16, 16)
  self.tileset = Bank.from_image("assets/tileset.img", 16, 16)

  self.animations = {
      ["sleeping-right"] = Animation.new(self.bank, { 12 }, 0, nil, false, false),
      ["sleeping-left"] = Animation.new(self.bank, { 12 }, 0, nil, true, false),
      ["idle-right"] = Animation.new(self.bank, { 9, 10, 11 }, 0.25, "circular", false, false),
      ["idle-left"] = Animation.new(self.bank, { 9, 10, 11 }, 0.25, "circular", true, false),
      ["running-right"] = Animation.new(self.bank, { 17, 18, 19, 20, 21, 22 }, 0.1, "circular", false, false),
      ["running-left"] = Animation.new(self.bank, { 17, 18, 19, 20, 21, 22 }, 0.1, "circular", true, false),
      ["jumping-right"] = Animation.new(self.bank, { 25 }, 0, nil, false, false),
      ["jumping-left"] = Animation.new(self.bank, { 25 }, 0, nil, true, false),
      ["falling-right"] = Animation.new(self.bank, { 26 }, 0, nil, false, false),
      ["falling-left"] = Animation.new(self.bank, { 26 }, 0, nil, true, false)
    }
  self.facing = "right"
  self.animation = self.animations["idle-" .. self.facing]
  self.idle_time = nil
  self.map = generate_map(50)
  self.shake_time = 5

  self.position = Vector2D.new(25 * 15 * 16, 0)
  self.velocity = Vector2D.new(0, 0)
  self.acceleration = Vector2D.new(0, -9.81 * 0.75)
  self.jumps = 0

  self.snow = {}
  self.flake_time = 0

  self.atlas:clear(0)

--  self.pixies:clear(0)
end

local function _extra_half_brite(palette, target, ratio)
  local r, g, b = table.unpack(target)
  local tweaked = Palette.new(palette)
  tweaked:lerp(r, g, b, ratio)
  palette:merge(64, tweaked, 0, 64, false) -- Just append.
--  local size = palette:size()
--  for index = 0, size - 1 do
--    local ar, ag, ab = palette:peek(index)
--    local mr, mg, mb = Palette.mix(r, g, b, ar, ag, ab, ratio)
--    palette:poke(size + index, mr, mg, mb)
--  end
  return palette
end

function Main:init()
  -- Tweak the palette now that the loading phase is complete, so that color-remapping won't be interfered with!
  local palette <const> = Palette.new(_extra_half_brite(PALETTE, { 31, 127, 63 }, 0.5))
  Display.palette(palette)
end

function Main:deinit()
end

function Main:handle_input()
  local vx, vy = self.velocity:unpack()
  if self.jumps < 2 and CONTROLLER:is_pressed("up") then
    vy = 128
    self.jumps = self.jumps + 1
    self.idle_time = nil
  elseif CONTROLLER:is_down("right") then
    self.facing = "right"
    vx = 64
    self.idle_time = nil
  elseif CONTROLLER:is_down("left") then
    self.facing = "left"
    vx = -64
    self.idle_time = nil
  elseif CONTROLLER:is_released("right") or CONTROLLER:is_released("left") then
    vx = 0
    self.idle_time = 0
  end
  self.velocity:assign(vx, vy)
  if CONTROLLER:is_pressed("start") then
    self.map = generate_map(50)
    self.shake_time = 5
  end
end

function Main:update(delta_time)
  self:handle_input()

  self.velocity:add(self.acceleration)
  self.position:fma(self.velocity, delta_time)

  local px, py = self.position:unpack()
  local vx, vy = self.velocity:unpack()

  if py <= 0 then
    py = 0
    vy = 0
    self.jumps = 0
  end

  self.position:assign(px, py)
  self.velocity:assign(vx, vy)

  local animation
  if vy > 0 then
    animation = self.animations["jumping-" .. self.facing]
  elseif vy < 0 then
    animation = self.animations["falling-" .. self.facing]
  elseif vx ~= 0 then
    animation = self.animations["running-" .. self.facing]
  elseif self.idle_time and self.idle_time >= 15 then
    animation = self.animations["sleeping-" .. self.facing]
  else
    animation = self.animations["idle-" .. self.facing]
  end
  if self.animation ~= animation then
    self.animation = animation
    self.animation:rewind()
  end

  self.animation:update(delta_time)

  if self.idle_time then
    self.idle_time = self.idle_time + delta_time
  end

  if self.shake_time > 0 then
    self.shake_time = self.shake_time * 0.5
    if self.shake_time < 0.01 then
      self.shake_time = 0
      Display.offset(0, 0)
    else
      local t = System.time()
      Display.offset(math.sin(t * 77 + 31) * 3, math.sin(t * 123 + 43) * 3)
    end
  end

  local canvas = Canvas.default()
  local width, height = canvas:image():size()

  if SNOW then
    self.flake_time = self.flake_time + delta_time
    while self.flake_time >= 0.025 do
      self.flake_time = self.flake_time - 0.025
      if #self.snow < 1024 then
        local v = 255
        local color = self.palette:match(v, v, v)
        table.insert(self.snow, {
            x = math.random(0, width - 1),
            y = -32,
            z = math.random(1, 5),
            angle = 0,
            vy = 24,
            vx = 0,
            va = math.random() * Math.SINCOS_PERIOD,
            color = color,
          })
      end
    end

    local wind_vx = 0 -- math.random(-128, 128)
    for index = #self.snow, 1, -1 do
      local flake = self.snow[index]
      local factor_x = -1.0 / flake.z
      local factor_y = 1.0 / flake.z
      flake.vx = self.velocity.x + wind_vx

      flake.x = flake.x + (flake.vx * delta_time) * factor_x
      flake.y = flake.y + (flake.vy * delta_time) * factor_y

      flake.angle = flake.angle + (flake.va * delta_time)

      if flake.x < 0 then
        flake.x = flake.x + width
      elseif flake.x >= width then
        flake.x = flake.x - width
      end

      if flake.y >= 96 then
        table.remove(self.snow, index)
      end
    end
  end

  local delta_y = py * 0.75
  local y = height * 0.5 + delta_y + 32

  local t = System.time()
  local program = Program.new()
  program:wait(0, y)
  for i = 0, 63 do
    program:shift(i, 64 + i)
  end
  program:modulo(-width * 2)
  for i = y, height - 1 do -- Combined x/y waves.
    program:wait(0, i)
    program:offset(math.cos(t * 6.0 + i * 0.125) * WATER_DISPLACEMENT)
    local d = - 1 - (math.sin(t * 2.5 + i * 0.25) * math.cos(t * 3.5 + i * 0.75) + 1)
    program:modulo(width * math.tointeger(d))
  end
  Display.program(program)
end

function Main:render(canvas, _)
  canvas:clear(50)

  local x, y = (WIDTH - 16) * 0.5, HEIGHT * 0.5

  local px, py = self.position:unpack()

  self.animation:render(canvas, x, y - py)

  local delta_y = py * 0.75

  y = y + delta_y

  local ox = px // 16
  local dx = px % 16
  for i = 1, 5 do
    for j = 1, 15 + 1 do
      local cell_id = self.map[i][ox + j]
      canvas:sprite((j - 1) * 16 - dx, y + 16 + (i - 2) * 16, self.tileset, cell_id)
    end
  end

  canvas:push()
  for _, flake in ipairs(self.snow) do
    canvas:shift(0, flake.color)
    local scale = 1--flake.z * 0.5
    canvas:sprite(flake.x, flake.y + delta_y, self.pixies, 0, scale, scale, flake.angle, 0.5, 0.5)
  end
  canvas:pop()

--[[
  local t = System.time()
  local mid = math.tointeger(y) + 32
  local amount = height - mid
  for i = 0, amount - 1 do
      canvas:process(function(_, _, _, to)
--        local ar, ag, ab = Display.get(from)
        local ar, ag, ab = 31, 127, 63
        local br, bg, bb = Display.get(to)
        local r, g, b = (ar + br) * 0.5, (ag + bg) * 0.5, (ab + bb) * 0.5
        return self.palette:match(r, g, b)
      end, 0, mid + i, math.sin(t + i / (amount / 8)) * 3, mid - i * 1, width, 1)
  end
]]
  canvas:write(0, 0, FONT, string.format("%d FPS", math.floor(System.fps() + 0.5)))

--  local a, b, c, d = System.stats()
--  self.font:write(string.format("%.2f %.2f %.2f %.2f %.2f", a, b, c, d, 1 / d), 0, 8)
end

return Main