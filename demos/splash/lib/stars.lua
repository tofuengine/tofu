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

local Class <const> = require("tofu.core.class")
local Math <const> = require("tofu.core.math")
local Bank <const> = require("tofu.graphics.bank")
local Arrays <const> = require("tofu.util.arrays")

local STAR_WIDTH <const> = 23
local STAR_HEIGHT <const> = 23

local STARS_SPAWN_INTERVAL <const> = 0.5
local MAX_STARS_COUNT <const> = 32

local START_ROTATION_SPEED_FACTOR <const> = 5.0

local Stars <const> = Class.define()

function Stars:__ctor(width, height, _, pool)
  local area <const> = { x0 = 0, y0 = 0, x1 = width - 1, y1 = height - 1 }

  self.pool = {}
  self.bank = Bank.from_image("assets/images/atlas.img", STAR_WIDTH, STAR_HEIGHT)
  self.timer = pool:spawn(STARS_SPAWN_INTERVAL, 0, function(_)
      if #self.pool >= MAX_STARS_COUNT then
        return
      end

      local depth <const> = math.random(1, 10) -- scale and speed depends of depth

      local scale <const> = depth * 0.5
      local vx <const> = scale * (7 + math.random(3, 5)) * (math.random(0, 1) > 0.5 and -1 or 1)
      local vy <const> = scale * (11 + math.random(3, 9))

      local w, h = STAR_WIDTH * scale * 0.5, STAR_HEIGHT * scale * 0.5

      local star = {
          cell_id = math.random(0, 19) > 18 and 0 or 1,
          scale = scale,
          area = { x0 = area.x0 - w, y0 = area.y0 - h, x1 = area.x1 + w, y1 = area.y1 + h },
          vx = vx, vy = vy,
          vr = vx * START_ROTATION_SPEED_FACTOR, -- Rotate in the "direction" of the Y movement, faster with speed.
          x = math.random(area.x0 - w, area.x1 + w),
          y = area.y0 - h,
          rotation = math.random(0, Math.SINCOS_PERIOD)
        }
      Arrays.add(self.pool, star, function(a, b) return a.scale < b.scale end)
    end)
end

function Stars:update(delta_time)
  for index = #self.pool, 1, -1 do -- Backward scan to easily remove while looping.
    local star = self.pool[index]
    star.x = star.x + star.vx * delta_time
    star.y = star.y + star.vy * delta_time
    star.rotation = star.rotation + star.vr * delta_time
    if star.x < star.area.x0 or star.x > star.area.x1 or star.y > star.area.y1 then
      table.remove(self.pool, index)
    end
  end
end

function Stars:render(canvas)
  for _, star in ipairs(self.pool) do
    canvas:sprite(star.x, star.y, self.bank, star.cell_id, star.scale, star.scale, star.rotation)
  end
end

return Stars
