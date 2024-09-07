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

Copyright (c) 2019-2024 Marco Lizza

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
local Bank = require("tofu.graphics.bank")
local Canvas = require("tofu.graphics.canvas")
local Display = require("tofu.graphics.display")
local Image = require("tofu.graphics.image")
local Palette = require("tofu.graphics.palette")
local Font = require("tofu.graphics.font")
local Controller = require("tofu.input.controller")
local World = require("tofu.physics.world")

local Bunny = require("lib/bunny")

local INITIAL_BUNNIES <const> = 1
local LITTER_SIZE <const> = 5
local MAX_BUNNIES <const> = 32768

local Main = Class.define()

function Main:__ctor()
  Display.palette(Palette.default("pico-8"))

  local canvas = Canvas.default()
  canvas:transparent({ ["0"] = false, ["11"] = true })

  local width, height = canvas:image():size()

  self.bunnies = {}
  self.bank = Bank.new(Image.new("assets/bunnies.png", 11), "assets/bunnies.sheet")
  self.font = Font.default(11, 6)

  self.world = World.new(0.0, 200.0)

  local left = self.world:spawn("box", 1, height, 0)
  left:type("static") -- Can either be kinematic or static (the latter is better)
  left:position(0, height * 0.5)
  left:elasticity(1.0)
  self.left = left

  local bottom = self.world:spawn("box", width, 1, 0)
  bottom:type("static")
  bottom:position(width * 0.5, height)
  bottom:elasticity(1.0)
  self.bottom = bottom

  local right = self.world:spawn("box", 1, height, 0)
  right:type("static")
  right:position(width, height * 0.5)
  right:elasticity(1.0)
  self.right = right

  for _ = 1, INITIAL_BUNNIES do
    table.insert(self.bunnies, Bunny.new(self.font, self.bank, self.world))
  end
end

function Main:init()
end

function Main:deinit()
end

function Main:handle_input()
  local controller = Controller.default()
  if controller:is_pressed("start") then
    for _ = 1, LITTER_SIZE do
      table.insert(self.bunnies, Bunny.new(self.font, self.bank, self.world))
    end
    if #self.bunnies >= MAX_BUNNIES then
      System.quit()
    end
  elseif controller:is_pressed("select") then
    for _, bunny in ipairs(self.bunnies) do
      self.world:remove(bunny.body)
    end
    self.bunnies = {}
  end
end

function Main:update(delta_time)
  self:handle_input()

  self.world:update(delta_time)
  for _, bunny in ipairs(self.bunnies) do
    bunny:update(delta_time)
  end
end

function Main:render(_)
  local canvas = Canvas.default()
  local image = canvas:image()
  local width, _ = image:size()
  image:clear(0)

  for _, bunny in ipairs(self.bunnies) do
    bunny:render(canvas)
  end

  canvas:write(0, 0, self.font, string.format("%d FPS", System.fps()))
  canvas:write(width, 0, self.font, string.format("#%d bunnies", #self.bunnies), "right")
end

return Main
