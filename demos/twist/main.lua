--[[
MIT License

Copyright (c) 2019-2022 Marco Lizza

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
local Image = require("tofu.graphics.image")
local Font = require("tofu.graphics.font")

local Main = Class.define()

function Main:__ctor()
  --Display.palette(Palette.default("pico-8"))

  local canvas = Canvas.default()
  local image = canvas:image()
  local width, height = image:size()

  self.bank = Bank.new(Image.new("assets/sheet.png", 0), 8, 8)
  self.font = Font.default(0, 255)

  local cw, ch = self.bank:size(Bank.NIL)
  self.columns = width / cw
  self.rows = height / ch
end

function Main:process()
end

function Main:update(_)
end

function Main:render(_)
  local canvas = Canvas.default()
  local image = canvas:image()
  local width, _ = image:size()
  image:clear(0)

  local time = System.time() * 7.5

  local cw, ch = self.bank:size(Bank.NIL)
  local x, y = (width - cw) * 0.5, 0

  for row = 1, self.rows do
--    local cell_id = math.tointeger((math.sin(time + row * 0.5) + 1) * 0.5 * 9)
    local cell_id = ((math.sin(time * 0.5 + row * 0.75) + 1) * 0.5) * 9
    canvas:sprite(x, y, self.bank, cell_id)
    y = y + ch
  end

  canvas:write(0, 0, self.font, string.format("FPS: %d", System.fps()))
end

return Main
