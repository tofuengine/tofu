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
local Bank = require("tofu.graphics.bank")
local Canvas = require("tofu.graphics.canvas")
local Display = require("tofu.graphics.display")
local Font = require("tofu.graphics.font")
local Palette = require("tofu.graphics.palette")
local Program = require("tofu.graphics.program")

local PALETTE <const> = Palette.default("nes")
local FONT <const> = Font.default()
local BIG_FONT <const> = Font.default("32x64")
local CANVAS <const> = Canvas.default()
local WIDTH <const>, HEIGHT <const> = CANVAS:image():size()
local CONTROLLER <const> = Controller.default()

local Main <const> = Class.define()

function Main:__ctor()
  self.bank = Bank.from_image("assets/sprites.img", 16, 16)
  self.running = true
  self.time = 0
  self.dx = 0
  self.dy = 0
  self.x = WIDTH * 0.5
  self.y = HEIGHT * 0.25
end

function Main:init()
  Display.palette(PALETTE)
end

function Main:deinit()
end

function Main:handle_input()
  if CONTROLLER:is_pressed("y") then
    self.running = not self.running
  end

  self.dx = 0
  self.dy = 0
  if CONTROLLER:is_down("up") then
    self.dy = self.dy - 1
  end
  if CONTROLLER:is_down("down") then
    self.dy = self.dy + 1
  end
  if CONTROLLER:is_down("left") then
    self.dx = self.dx - 1
  end
  if CONTROLLER:is_down("right") then
    self.dx = self.dx + 1
  end
end

function Main:update(delta_time)
  self:handle_input()

  if not self.running then
    return
  end

  self.time = self.time + delta_time

  self.x = self.x + self.dx * 64 * delta_time
  self.y = self.y + self.dy * 64 * delta_time

  local t = self.time
  local y = math.sin(t * 2.5) * HEIGHT * 0.125 + HEIGHT * 0.25

  local program = Program.new()
  program:wait(0, 0)
  program:color(0, 0x00, 0x00, 0x00)
  program:color(1, 0x00, 0x00, 0x00)
  program:color(31, 0x00, 0x00, 0x00)
  for i = 15, 0, -1 do
    local v = 0x11 * (15 - i)
    program:wait(0, y - i)
    program:color(0, v, 0x00, 0x00)
    program:color(1, v, 0x00, v)
    program:color(31, v, v, 0x00)
  end
  for i = 0, 15 do
    local v = 0x11 * (15 - i)
    program:wait(0, y + i)
    program:color(0, v, 0x00, 0x00)
    program:color(1, v, 0x00, v)
    program:color(31, v, v, 0x00)
  end
  program:wait(0, HEIGHT * 0.5)
  program:color(0, 0x00, 0x11, 0x44)
  program:color(1, 0x00, 0x11, 0x44)
  program:color(31, 0x00, 0x11, 0x44)
  program:modulo(-WIDTH * 2)
  for i = HEIGHT // 2, HEIGHT - 1 do
    program:wait(0, i)
    program:offset(math.sin(t * 13.0 + i * 0.25) * 1.5)
  end
  Display.program(program)
end

function Main:render(canvas, _)
  canvas:clear(0)

  canvas:push()
    canvas:shift(0, 1)
    local t = self.time
    local y = math.sin(t * 0.5) * HEIGHT * 0.125 + HEIGHT * 0.25
    canvas:write(0, y, BIG_FONT, "TOFU ENGINE")
  canvas:pop()

  canvas:sprite(self.x, self.y, self.bank, 12, 4, 4, 0)

  canvas:push()
    canvas:shift(0, 11)
    canvas:write(0, 0, FONT, string.format("%d FPS", System.fps()))
  canvas:pop()
end

return Main