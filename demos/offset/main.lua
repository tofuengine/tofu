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
local Canvas = require("tofu.graphics.canvas")
local Display = require("tofu.graphics.display")
local Image = require("tofu.graphics.image")
local Font = require("tofu.graphics.font")
local Palette = require("tofu.graphics.palette")
local Controller = require("tofu.input.controller")

local Main = Class.define()

function Main:__ctor()
  Display.palette(Palette.default("pico-8"))

  self.font = Font.default(0, 1)

  local canvas = Canvas.default()
  local image = canvas:image()
  local width, height = image:size()

  self.buffer = Canvas.new(Image.new(width, height))

  for x = 0, width - 1 do
    self.buffer:line(x, 0, x, height - 1, x % 16)
  end

  self.offset = { x = 0, y = 0 }
end

function Main:init()
end

function Main:deinit()
end

function Main:handle_input()
  local controller = Controller.default()
  if controller:is_pressed("down") then
    self.offset.y = self.offset.y + 1
  elseif controller:is_pressed("up") then
    self.offset.y = self.offset.y - 1
  elseif controller:is_pressed("right") then
    self.offset.x = self.offset.x + 1
  elseif controller:is_pressed("left") then
    self.offset.x = self.offset.x - 1
  end

  Display.offset(self.offset.x, self.offset.y)
end

function Main:update(_)
  self:handle_input()
end

function Main:render(_) -- ratio
  local canvas = Canvas.default()

  canvas:copy(self.buffer:image())

  canvas:write(0, 0, self.font, string.format("%d FPS", System.fps()))
end

return Main
