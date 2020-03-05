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
local Input = require("tofu.events").Input
local Canvas = require("tofu.graphics").Canvas
local Font = require("tofu.graphics").Font

local Map = require("lib.map")

local Main = Class.define()

local CAMERA_SPEED = 128.0

--[[
function string.split(s, sep)
  local fields = {}
  local pattern = string.format("([^%s]+)", sep or " ")
  string.gsub(s, pattern, function(c) fields[#fields + 1] = c end)
  return fields
end

function table.dump(t, spaces)
  spaces = spaces or ""
  for k, v in pairs(t) do
    print(spaces .. k .. " " .. type(v) .. " " .. tostring(v))
    if type(v) == "table" then
      if (k ~= "__index") then
        dump(v, spaces .. " ")
      end
    end
  end
end
]]--
--[[
local function roundm(n, m)
  return math.floor(((n + m - 1) / m)) * m
end

local function getRandomPointInEllipse(ellipse_width, ellipse_height)
  local t = 2*math.pi*math.random()
  local u = math.random()+math.random()
  local r = nil
  if u > 1 then r = 2-u else r = u end
  return roundm(ellipse_width*r*math.cos(t)/2, tile_size),
         roundm(ellipse_height*r*math.sin(t)/2, tile_size)
end

local function random_point_in_circle(radius)
  local t =  2 * math.pi * math.random()
  local u = math.random()+math.random()
  local r = nil
  if u > 1 then
    r = 2 - u
  else
    r = u
  end
  return radius * r * math.cos(t), radius * r * math.sin(t)
end

local function generate(width, height)
  local grid = Grid.new(width, height)

  local rw, rh = width * 0.125, height * 0.125

  local hw, hh = width * 0.5, height * 0.5
  local radius = math.min(hw, hh)

  for _ = 1, 100 do
    local x, y = random_point_in_circle(radius)
    x = x + hw
    y = y + hh
    local w = math.rand() * rw + rw
    local h = math.rand() * rh + rh
  end
end
]]--

function Main:__ctor()
  Canvas.palette("gameboy")

  self.font = Font.default(3, 1)
  self.map = Map.from_file("assets/world.map")

  self.map:add_camera("left", 7, 5, 8, 0)
  self.map:add_camera("right", 7, 5, 248, 0, 0.5, 0.5, 0.25)
  self.map:add_camera("main", 14, 5, 8, 160)

  self.player = { x = 640, y = 640 }

  self.map:camera_from_id("left"):move_to(200, 200)
  self.map:camera_from_id("right"):move_to(800, 200)
  for _, camera in pairs(self.map:get_cameras()) do
    camera.post_draw = function(me)
        local x, y = me:to_screen(me.x, me.y)
        Canvas.rectangle("fill", x - 2, y - 2, 4, 4, 2)
        self.font:write(self.font:align(tostring(me), me.screen_x + me.screen_width, me.screen_y, "right"))
      end
  end
end

function Main:input()
  self.dx = 0
  self.dy = 0
  if Input.is_down("left") then
    self.dx = self.dx - 1
  end
  if Input.is_down("right") then
    self.dx = self.dx + 1
  end
  if Input.is_down("up") then
    self.dy = self.dy - 1
  end
  if Input.is_down("down") then
    self.dy = self.dy + 1
  end
end

function Main:update(delta_time)
  local camera = self.map:camera_from_id("left")

  local t = System.time() * 0.5
  local c, s = math.cos(t), math.sin(t)
  local ax = (c + 1) * 0.5 + 0.0 -- [0.25, 0.75]
  local ay = (s + 1) * 0.5 + 0.0
  camera:center_at(ax, ay)

  camera = self.map:camera_from_id("right")
  local dx = self.dx * CAMERA_SPEED * delta_time
  local dy = self.dy * CAMERA_SPEED * delta_time
  if dx ~= 0.0 or dy ~= 0.0 then
    self.player.x, self.player.y = self.map:bound(self.player.x + dx, self.player.y + dy)
  end
  camera:move_to(self.player.x, self.player.y)
--[[
  camera = self.map:camera_from_id("main")
  if dx ~= 0.0 or dy ~= 0.0 then
    self.player.x, self.player.y = self.map:bound(self.player.x + dx, self.player.y + dy)
    camera:move_to(self.player.x, self.player.y)
  end
]]
  self.map:update(delta_time)
end

function Main:render(_)
  Canvas.clear()
  self.map:draw()

  local camera = self.map:camera_from_id("right")
  local x, y = camera:to_screen(self.player.x, self.player.y)
  Canvas.rectangle("fill", x - 2, y - 2, 4, 4, 1)

  self.font:write(string.format("FPS: %d", System.fps()), 0, 0)
end

return Main