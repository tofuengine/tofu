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
local Log = require("tofu.core.log")
local System = require("tofu.core.system")
local Controller = require("tofu.input.controller")
local Canvas = require("tofu.graphics.canvas")
local Display = require("tofu.graphics.display")
local Font = require("tofu.graphics.font")
local Palette = require("tofu.graphics.palette")

local bump = require("lib/bump")

local PALETTE <const> = Palette.default("famicube")
local FONT <const> = Font.default(63, 11)

local LIFE <const> = 2.0

local Main = Class.define()

-- Block functions
function Main:_add_block(x, y, w, h)
  local block = { x = x, y = y, w = w, h = h, life = 0 }
  self.blocks[#self.blocks + 1] = block
  self.world:add(block, x, y, w, h)
end

function Main:__ctor()
  Display.palette(PALETTE)

  local canvas = Canvas.default()
  canvas:transparent({ [0] = false, [63] = true })

  self.player = { x = 50, y = 50, w = 20, h = 20, speed = 80 }

  -- World creation
  self.world = bump.newWorld()
  self.world:add(self.player, self.player.x, self.player.y, self.player.w, self.player.h)

  self.blocks = {}
  self:_add_block(0,       0,     800, 32)
  self:_add_block(0,      32,      32, 600-32*2)
  self:_add_block(800-32, 32,      32, 600-32*2)
  self:_add_block(0,      600-32, 800, 32)

  for _ = 1, 30 do
    self:_add_block(math.random(100, 600),
                    math.random(100, 400),
                    math.random(10, 100),
                    math.random(10, 100)
    )
  end
end

function Main:process()
  local controller = Controller.default()
  if controller:is_pressed("start") then
    collectgarbage("collect")
  end

  if controller:is_down("right") then
    self.dx = 1
  elseif controller:is_down("left") then
    self.dx = -1
  else
    self.dx = 0
  end
  if controller:is_down("down") then
    self.dy = 1
  elseif controller:is_down("up") then
    self.dy = -1
  else
    self.dy = 0
  end
end

function Main:update_player(delta_time)
  local speed = self.player.speed
  local dx, dy = self.dx * speed * delta_time, self.dy * speed * delta_time

  if dx ~= 0 or dy ~= 0 then
    local cols, cols_len
    self.player.x, self.player.y, cols, cols_len = self.world:move(self.player,
      self.player.x + dx, self.player.y + dy, function(_, _) return "slide" end)
    for i = 1, cols_len do
      local col = cols[i]
      col.other.life = LIFE
      Log.info(("c.other = %s, c.type = %s, c.normal = %d,%d"):format(col.other, col.type, col.normal.x, col.normal.y))
    end
  end
end

function Main:update_blocks(delta_time)
  for _, block in ipairs(self.blocks) do
    if block.life > 0 then
      block.life = math.max(0, block.life - delta_time)
    end
  end
end

function Main:update(delta_time)
  self:update_player(delta_time)
  self:update_blocks(delta_time)
end

-- helper function
local function _box(canvas, box, r, g, b)
  canvas:rectangle('fill', box.x, box.y, box.w, box.h, PALETTE:match(r * 0.5, g * 0.5, b * 0.5))
  canvas:rectangle('line', box.x, box.y, box.w, box.h, PALETTE:match(r, g, b))
end

function Main:draw_blocks(canvas)
  for _, block in ipairs(self.blocks) do
    if block.life > 0 then
      _box(canvas, block, 255, 255, 0)
    else
      _box(canvas, block, 255, 0, 0)
    end
  end
end

-- Player functions
function Main:draw_player(canvas)
  _box(canvas, self.player, 0, 255, 0)
end

function Main:render(_)
  local canvas = Canvas.default()
  canvas:clear(0)

  self:draw_blocks(canvas)
  self:draw_player(canvas)

  canvas:write(0, 0, FONT, string.format("%d FPS", System.fps()))
end

return Main
