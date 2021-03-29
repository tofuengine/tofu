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

-- Include the modules we'll be using.
local Class = require("tofu.core").Class
local Math = require("tofu.core").Math
local System = require("tofu.core").System
local Bank = require("tofu.graphics").Bank
local Canvas = require("tofu.graphics").Canvas
local Copperlist = require("tofu.graphics").Copperlist
local Display = require("tofu.graphics").Display
local Palette = require("tofu.graphics").Palette
local Font = require("tofu.graphics").Font
local Source = require("tofu.sound").Source
local Timer = require("tofu.timers").Timer

local STAR_PERIOD = 1.0 / 3.0

local Main = Class.define()

function Main:__ctor()
  local palette = Palette.new("famicube")
  Display.palette(palette)

  local canvas = Canvas.default()
  canvas:background(63) -- Use index #63 as background color and transparent! :)
  canvas:transparent({ [0] = false, [63] = true })

  self.bank = Bank.new(canvas, Canvas.new("assets/images/atlas.png", 63), 23, 23)
  self.font = Font.new(canvas, Canvas.new("assets/images/font-8x8.png", 31, 0), 8, 8)

  self.images = {
      Canvas.new("assets/images/tofu.png", 63),
      Canvas.new("assets/images/engine.png", 63)
    }
  self.outline = Canvas.new("assets/images/outline.png", 63)
  self.stripe = Canvas.new("assets/images/stripes.png", 63)

  self.timer = Timer.new(10, 0, function(_)
    local _, height = canvas:size()
    local half_height = math.tointeger(height / 2)
    local quarter_height = math.tointeger(height / 4)
    local copperlist = Copperlist.gradient(63, {
        { 0, palette:index_to_color(math.random(0, 63)) },
        { quarter_height - 1, palette:index_to_color(math.random(0, 63)) },
        { half_height - 1, palette:index_to_color(math.random(0, 63)) },
        { height - quarter_height - 1, palette:index_to_color(math.random(0, 63)) },
        { height - 1, palette:index_to_color(math.random(0, 63)) }
      })
--    copperlist:wait(0, height - math.tointeger(quarter_height / 2) - 1)
--    copperlist:modulo(-width * 4)
    Display.copperlist(copperlist)
  end)

  self.wave = {
      tweener = Math.tweener("linear"),
      period_current = 0,
      period_next = 0,
      period = 0,
      update = function(this, delta_time)
          local period = Math.lerp(this.period_current, this.period_next, this.tweener(this.timer.age / 5))
          this.period = this.period + delta_time * period
        end
    }
  self.wave.timer = Timer.new(5, 0, function(_)
      self.wave.period_current = self.wave.period_next
      self.wave.period_next = math.random() * 2.0 + 1.5
    end)

  self.tick = 0
  self.stars = { }

  self.music = Source.new("assets/modules/a_nice_and_warm_day.mod", Source.MODULE)
  self.music:looped(true)
  self.music:play()
end

function Main:input()
end

function Main:update(delta_time)
  self.wave:update(delta_time)

  self.tick = self.tick + delta_time
  while self.tick > STAR_PERIOD do
    self.tick = self.tick - STAR_PERIOD
    if #self.stars < 64 then
      local vx = math.random(-5, 5) * 7
      local vy = math.random(3, 5) * 11
      table.insert(self.stars, {
          cell_id = math.random(0, 19) > 18 and 0 or 1,
          x = math.random(-32, 479 + 32),
          y = -32,
          scale = math.random(1, 10) * 0.5,
          rotation = 0,
          vx = vx, vy = vy,
          vr = vx * 7.0 -- Rotate in the "direction" of the Y movement, faster with speed.
        })

      table.sort(self.stars, function(a, b) return a.scale < b.scale end)
    end
  end

  for index = #self.stars, 1, -1 do
    local star = self.stars[index]
    local size = 24 * star.scale
    star.x = star.x + star.vx * delta_time
    star.y = star.y + star.vy * delta_time
    star.rotation = star.rotation + star.vr * delta_time
    if star.x < -size or star.x > (480 + size) or star.y > (320 + size) then
      table.remove(self.stars, index)
    end
  end
end

function Main:render(_)
  local t = System.time()

  local canvas = Canvas.default()
  canvas:clear()

  local canvas_width, _ = canvas:size()
  local stripe_width, stripe_height = self.stripe:size()
  for i = 0, canvas_width, stripe_width do
    local dy = math.sin(self.wave.period + i * 0.0025) * stripe_height * 0.25

    self.stripe:copy(i, dy, canvas)
  end

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
  self.outline:copy(0, 0, canvas)
  local index = (math.tointeger(t * 1.0) % 2) + 1
  local image = self.images[index]
  image:copy(0, 0, canvas)

  for _, star in ipairs(self.stars) do
    self.bank:blit(star.cell_id, star.x, star.y, star.scale, star.scale, star.rotation)
  end
end

return Main
