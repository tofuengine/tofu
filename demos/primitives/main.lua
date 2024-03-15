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
local Controller = require("tofu.input.controller")
local Canvas = require("tofu.graphics.canvas")
local Display = require("tofu.graphics.display")
local Font = require("tofu.graphics.font")
local Palette = require("tofu.graphics.palette")

local Main = Class.define()

local PALETTE <const> = {
    { 0x00, 0x00, 0x00 }, { 0x24, 0x00, 0x00 }, { 0x48, 0x00, 0x00 }, { 0x6D, 0x00, 0x00 },
    { 0x91, 0x00, 0x00 }, { 0xB6, 0x00, 0x00 }, { 0xDA, 0x00, 0x00 }, { 0xFF, 0x00, 0x00 },
    { 0xFF, 0x3F, 0x00 }, { 0xFF, 0x7F, 0x00 }, { 0xFF, 0xBF, 0x00 }, { 0xFF, 0xFF, 0x00 },
    { 0xFF, 0xFF, 0x3F }, { 0xFF, 0xFF, 0x7F }, { 0xFF, 0xFF, 0xBF }, { 0xFF, 0xFF, 0xFF }
  }

local FOREGROUND <const> = 3

function Main:__ctor()
  Display.palette(Palette.new(PALETTE)) -- "arne-16")
  Display.palette(Palette.default("arne-16"))

  self.font = Font.default(0, 1)
  self.mode = 0
end

function Main:handle_input()
  local controller = Controller.default()
  if controller:is_pressed("start") then
    System.quit()
  elseif controller:is_pressed("right") then
    self.mode = (self.mode % 10) + 1
  elseif controller:is_pressed("left") then
    self.mode = ((self.mode + 8) % 10) + 1
  end
end

function Main:update(_) -- delta_time
  self:handle_input()
end

function Main:render(_) -- ratio
  local canvas = Canvas.default()
  local image = canvas:image()
  image:clear(0)

  local width, height = image:size()

  if self.mode == 0 then
    local cx, cy = 8, 32
    for r = 0, 12 do
      canvas:circle("fill", cx, cy, r, FOREGROUND)
      canvas:circle("line", cx, cy + 64, r, FOREGROUND)
      cx = cx + 2 * r + 8
    end

    canvas:polyline({ 64, 64, 64, 128, 128, 128 }, FOREGROUND)

    local x0 = (math.random() * width * 2) - width * 0.5
    local y0 = (math.random() * width * 2) - width * 0.5
    local x1 = (math.random() * width * 2) - width * 0.5
    local y1 = (math.random() * width * 2) - width * 0.5
    canvas:line(x0, y0, x1, y1, FOREGROUND)
  elseif self.mode == 1 then
    local dx = math.cos(System.time()) * 32
    local dy = math.sin(System.time()) * 32
    canvas:circle("fill", 128, 64, 32, 1)
    canvas:line(128, 64, 128 + dx, 64 + dy, 2)
  elseif self.mode == 2 then
    canvas:triangle("fill", 5, 50, 5, 150, 150, 150, 1)
    canvas:triangle("fill", 5, 50, 150, 50, 150, 150, 3)
  elseif self.mode == 3 then
    local x0 = ((math.cos(System.time() * 0.125) + 1.0) * 0.5) * width
    local y0 = ((math.cos(System.time() * 0.342) + 1.0) * 0.5) * height
    local x1 = ((math.sin(System.time() * 0.184) + 1.0) * 0.5) * width
    local y1 = ((math.sin(System.time() * 0.223) + 1.0) * 0.5) * height
    local x2 = ((math.cos(System.time() * 0.832) + 1.0) * 0.5) * width
    local y2 = ((math.sin(System.time() * 0.123) + 1.0) * 0.5) * height
    canvas:triangle("fill", x0, y0, x1, y1, x2, y2, 2)
    canvas:triangle("line", x0, y0, x1, y1, x2, y2, 7)
    canvas:push()
      canvas:shift(1, 15)
      canvas:write(x0, y0, self.font, "v0", "center", "middle")
      canvas:write(x1, y1, self.font, "v1", "center", "middle")
      canvas:write(x2, y2, self.font, "v2", "center", "middle")
    canvas:pop()
  elseif self.mode == 4 then
    local x = ((math.cos(System.time() * 0.125) + 1.0) * 0.5) * width
    local y = ((math.cos(System.time() * 0.342) + 1.0) * 0.5) * height
    canvas:square("fill", x, y, 75, 2)
    canvas:square("line", 96, 96, 64, 2)
  elseif self.mode == 5 then
    local cx = width * 0.5
    local cy = height * 0.5
    canvas:circle("fill", cx, cy, 50, 3)
    canvas:circle("line", cx, cy, 50, 4)
  elseif self.mode == 6 then
    local cx = width * 0.5
    local cy = height * 0.5
    canvas:circle("line", cx, cy, 50, 4)
    canvas:circle("fill", cx, cy, 50, 3)
  elseif self.mode == 7 then
    local cx = ((math.cos(System.time() * 0.125) + 1.0) * 0.5) * width
    local cy = ((math.cos(System.time() * 0.342) + 1.0) * 0.5) * height
    local r = ((math.sin(System.time() * 0.184) + 1.0) * 0.5) * 63 + 1
    canvas:circle("fill", cx, cy, r, 6)
  elseif self.mode == 8 then
    local cx = ((math.cos(System.time() * 0.125) + 1.0) * 0.5) * width
    local cy = ((math.cos(System.time() * 0.342) + 1.0) * 0.5) * height
    local r = ((math.sin(System.time() * 0.184) + 1.0) * 0.5) * 63 + 1
    canvas:circle("line", cx, cy, r, 7)
  elseif self.mode == 9 then
    local colors = { 13, 11, 9, 7, 5, 3, 1 }
    local y = (math.sin(System.time()) + 1.0) * 0.5 * height
    canvas:hline(0, y, width - 1, 15)
    for i, c in ipairs(colors) do
      canvas:hline(0, y - i, width - 1, c)
      canvas:hline(0, y + i, width - 1, c)
    end
  elseif self.mode == 10 then
    canvas:point(4, 4, 1)
    canvas:line(8, 8, 32, 32, 2)
    canvas:rectangle("line", 4, 23, 8, 8, 3)
    canvas:triangle("line", 150, 150, 50, 250, 250, 250, 3)
    canvas:rectangle("fill", 4, 12, 8, 8, 3)
  end

  canvas:write(0, 0, self.font, string.format("%d FPS", System.fps()))
  canvas:write(width, 0, self.font, string.format("mode: %d", self.mode), "right")
end

return Main
