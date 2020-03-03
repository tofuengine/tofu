--[[
MIT License

Copyright (c) 2019-2020 Marco Lizza

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
local Bank = require("tofu.graphics").Bank
local Canvas = require("tofu.graphics").Canvas
local Font = require("tofu.graphics").Font

local Main = Class.define()

function Main:__ctor()
  --Display.palette("pico-8")

  local canvas = Canvas.default()
  local width, height = canvas:size()

  self.bank = Bank.new("assets/sheet.png", 8, 8)
  self.font = Font.default(0, 255)

  local cw, ch = self.bank:size(-1)
  self.columns = width / cw
  self.rows = height / ch
end

function Main:input()
end

function Main:update(_)
end

function Main:render(_)
  local canvas = Canvas.default()
  local width, _ = canvas:size()
  canvas:clear()

  local time = System.time() * 7.5

  local cw, ch = self.bank:size(-1)
  local x, y = (width - cw) * 0.5, 0

  for row = 1, self.rows do
--    local cell_id = math.tointeger((math.sin(time + row * 0.5) + 1) * 0.5 * 9)
    local cell_id = math.tointeger(time + row * 0.25) % 9
    self.bank:blit(cell_id, x, y)
    y = y + ch
  end

  self.font:write(string.format("FPS: %d", System.fps()), 0, 0)
end

return Main
