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
---local Controller = require("tofu.input.controller")
local Canvas = require("tofu.graphics.canvas")
local Display = require("tofu.graphics.display")
local Font = require("tofu.graphics.font")
local Palette = require("tofu.graphics.palette")
--local Arrays = require("tofu.util.arrays")
local Vector2D = require("tofu.util.vector2d")

local Boid = require('lib.boid')
--local Obstacle = require('lib.obstacle')
local Rules = require('lib.rules')

local PALETTE <const> = Palette.default("pico-8")
local FONT <const> = Font.default()
local CANVAS <const> = Canvas.default()
local WIDTH <const>, HEIGHT <const> = CANVAS:image():size()

local BOIDS <const> = 64
--local OBSTACLES_PADDING <const> = 16
local INFLUENCE_RADIUS <const> = 48
local FOV <const> = math.pi / 4 * 3

local RULES <const> = {
  { rule = Rules.alignment, weight = 4 },
  { rule = Rules.cohesion, weight =  2 },
  { rule = Rules.separation, weight = 3 },
  { rule = Rules.follow, weight = 1 },
  -- TODO: scattering
  -- TODO: perching
}

local Main = Class.define()

local function spawn(objects)
  local x = math.random(0, WIDTH - 1)
  local y = math.random(0, HEIGHT - 1)
  local angle = math.random() * 2 * math.pi
  table.insert(objects, Boid.new(Vector2D.new(x, y), angle, FOV, INFLUENCE_RADIUS))
end

-- local function kill(objects)
--   Arrays.erase_if(objects,
--     function(value, index, length, array)
--       if not value.is_obstacle then
--         return true, true -- delete only the first boid we find
--       end
--     end)
-- end

function Main:__ctor()
  --self.is_profiled = true

  self.objects = {}

  for _ = 1, BOIDS do
    spawn(self.objects)
  end
end

function Main:init()
  Display.palette(PALETTE)
end

function Main:deinit()
end

function Main:handle_input()
end

function Main:update(delta_time)
  self:handle_input()

  local velocities = {}
  for _, object in ipairs(self.objects) do
    local flockmates = object:find_flockmates(self.objects)

    local velocity = Vector2D.new()
    for _, rule in ipairs(RULES) do
      velocity:add(rule.rule(object, flockmates, { weight = rule.weight }))
    end
    velocities[object] = velocity
  end

  for object, velocity in pairs(velocities) do
    object:update(velocity, delta_time)
  end
end

function Main:render(canvas, _)
  for _, object in ipairs(self.objects) do
    object:draw(canvas)
  end

  canvas:write(0, 0, FONT, string.format("%d FPS", System.fps()))
  canvas:write(WIDTH, 0, FONT, string.format("#%d objects", #self.objects), "right")
end

return Main