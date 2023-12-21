--[[
MIT License

Copyright (c) 2019-2023 Marco Lizza

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
local Bank = require("tofu.graphics.bank")
local Batch = require("tofu.graphics.batch")
local Image = require("tofu.graphics.image")
local Arrays = require("tofu.util.arrays")

local STAR_WIDTH = 23
local STAR_HEIGHT = 23

local MAX_STARS_COUNT = 32

local Stars = Class.define()

function Stars:__ctor(width, height, transparent, _, pool)
  local area = { x0 = 0, y0 = 0, x1 = width - 1, y1 = height - 1 }

  self.pool = {}
  self.bank = Bank.new(Image.new("assets/images/atlas.png", transparent), STAR_WIDTH, STAR_HEIGHT)
  self.batch = Batch.new(self.bank, MAX_STARS_COUNT)
  self.timer = pool:spawn(0.5, 0, function(_)
      if #self.pool >= MAX_STARS_COUNT then
        return
      end

      local scale = math.random(1, 10) * 0.5
      local vx = math.random(-5, 5) * 7
      local vy = math.random(3, 5) * 11

      local w, h = STAR_WIDTH * scale * 0.5, STAR_HEIGHT * scale * 0.5

      local star = {
          cell_id = math.random(0, 19) > 18 and 0 or 1,
          scale = scale,
          area = { x0 = area.x0 - w, y0 = area.y0 - h, x1 = area.x1 + w, y1 = area.y1 + h },
          vx = vx, vy = vy,
          vr = vx * 7.0, -- Rotate in the "direction" of the Y movement, faster with speed.
          x = math.random(area.x0 - w, area.x1 + w),
          y = area.y0 - h,
          rotation = math.random(0, Math.SINCOS_PERIOD)
        }
      Arrays.add(self.pool, star, function(a, b) return a.scale < b.scale end)
    end)
end

function Stars:update(delta_time)
  self.batch:clear()

  for index = #self.pool, 1, -1 do -- Backward scan to easily remove while looping.
    local star = self.pool[index]
    star.x = star.x + star.vx * delta_time
    star.y = star.y + star.vy * delta_time
    star.rotation = star.rotation + star.vr * delta_time
    if star.x < star.area.x0 or star.x > star.area.x1 or star.y > star.area.y1 then
      table.remove(self.pool, index)
    else
      self.batch:add(star.cell_id, star.x, star.y, star.scale, star.scale, star.rotation)
    end
  end
end

function Stars:render(canvas)
  canvas:flush(self.batch, 'complete')
end

return Stars
