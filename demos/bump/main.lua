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
local Log = require("tofu.core").Log
local System = require("tofu.core").System
local Input = require("tofu.events").Input
local Canvas = require("tofu.graphics").Canvas
local Display = require("tofu.graphics").Display
local Font = require("tofu.graphics").Font
local Palette = require("tofu.graphics").Palette

local bump = require("lib/bump")

local cols_len = 0 -- how many collisions are happening

-- World creation
local world = bump.newWorld()

local palette = Palette.new("famicube")
local font = Font.default(63, 11)

-- helper function
local function drawBox(canvas, box, r, g, b)
  canvas:rectangle('fill', box.x, box.y, box.w, box.h, palette:match(r * 0.5, g * 0.5, b * 0.5))
  canvas:rectangle('line', box.x, box.y, box.w, box.h, palette:match(r, g, b))
end

-- Player functions
local player = { x=50,y=50,w=20,h=20, speed = 80 }

local function updatePlayer(dt)
  local speed = player.speed

  local dx, dy = 0, 0
  if Input.is_down('right') then
    dx = speed * dt
  elseif Input.is_down('left') then
    dx = -speed * dt
  end
  if Input.is_down('down') then
    dy = speed * dt
  elseif Input.is_down('up') then
    dy = -speed * dt
  end

  if dx ~= 0 or dy ~= 0 then
    local cols
    player.x, player.y, cols, cols_len = world:move(player, player.x + dx, player.y + dy,
      function(_, _) return 'slide' end)
    for i=1, cols_len do
      local col = cols[i]
      Log.info(("c.other = %s, c.type = %s, c.normal = %d,%d"):format(col.other, col.type, col.normal.x, col.normal.y))
    end
  end
end

local function drawPlayer(canvas)
  drawBox(canvas, player, 0, 255, 0)
end

-- Block functions

local blocks = {}

local function addBlock(x,y,w,h)
  local block = {x=x,y=y,w=w,h=h}
  blocks[#blocks+1] = block
  world:add(block, x,y,w,h)
end

local function drawBlocks(canvas)
  for _,block in ipairs(blocks) do
    drawBox(canvas, block, 255,0,0)
  end
end

local Main = Class.define()

function Main:__ctor()
  Display.palette(palette)

  local canvas = Canvas.default()
  canvas:transparent({ [0] = false, [63] = true })

  world:add(player, player.x, player.y, player.w, player.h)

  addBlock(0,       0,     800, 32)
  addBlock(0,      32,      32, 600-32*2)
  addBlock(800-32, 32,      32, 600-32*2)
  addBlock(0,      600-32, 800, 32)

  for _ = 1, 30 do
    addBlock( math.random(100, 600),
              math.random(100, 400),
              math.random(10, 100),
              math.random(10, 100)
    )
  end
end

function Main:process()
  if Input.is_pressed("start") then
    collectgarbage("collect")
  end
end

function Main:update(delta_time)
  cols_len = 0
  updatePlayer(delta_time)
end

function Main:render(_)
  local canvas = Canvas.default()
  canvas:clear(0)

  drawBlocks(canvas)
  drawPlayer(canvas)

  font:write(canvas, 0, 0, string.format("FPS: %d", System.fps()))
end

return Main
