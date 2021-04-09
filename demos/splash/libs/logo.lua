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
local Canvas = require("tofu.graphics").Canvas

local Logo = Class.define()

function Logo:__ctor(canvas, transparent)
  self.canvas = canvas
  self.images = {
    Canvas.new("assets/images/tofu.png", transparent),
    Canvas.new("assets/images/engine.png", transparent)
  }
  self.outline = Canvas.new("assets/images/outline.png", transparent)
end

function Logo:update(_)
end

function Logo:render()
--[[
  canvas:push()
  local x, y = 0, 0
  local dy = 0
  local message = '* T O F U - E N G I N E '
  local colors = { 0, 5, 6, 10, 7, 23, 6, 5 }
  local char_width, char_height = self.font:size()
  local offset = 0 -- math.tointeger(t * 17.0)
  local cursor = math.tointeger(t * 5.0)
  for k = 0, 49 do
    local dx = 0
    for i = 0, 59 do
      local j = ((cursor + k + i) % #message) + 1
      local c = message:sub(j, j)
      canvas:shift(0, colors[(i + offset) % #colors + 1])
      self.font:write(c, x + dx, y + dy)
      dx = dx + char_width
    end
    dy = dy + char_height
  end
  canvas:pop()
--]]

  -- TODO: generate the outline w/ 4 offsetted blits.
  local t = System.time()
  self.outline:blit(self.canvas)
  local index = (math.tointeger(t * 1.0) % 2) + 1
  local image = self.images[index]
  image:blit(self.canvas)
end

return Logo
