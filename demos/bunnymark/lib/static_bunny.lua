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

local Bunny = Class.define()

local CELL_ID = 0
local MIN_X, MIN_Y = 0, 0
local MAX_X, MAX_Y = Canvas.default():size()

function Bunny:__ctor(bank, batch)
  local cw, ch = bank:size(CELL_ID)

  local min_x = MIN_X + cw
  local min_y = MIN_Y + cw
  local max_x = MAX_X - cw * 2
  local max_y = MAX_Y - ch * 2

  self.batch = batch
  self.x = math.random() * (max_x - min_x) + cw
  self.y = math.random() * (max_y - min_y) + ch

  self.batch:add(CELL_ID, self.x, self.y)
end

function Bunny:update(_)
  self.batch:add(CELL_ID, self.x, self.y)
end

return Bunny