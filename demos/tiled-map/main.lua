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
local Input = require("tofu.events").Input
local Display = require("tofu.graphics").Display
local Canvas = require("tofu.graphics").Canvas
local Font = require("tofu.graphics").Font
local Palette = require("tofu.graphics").Palette

local Map = require("lib.map")

local Main = Class.define()

local CAMERA_SPEED = 128.0

function Main:__ctor()
  Display.palette(Palette.new("gameboy"))

  self.font = Font.default(3, 1)
  self.map = Map.from_file("assets/world.map")

  self.map:add_camera("left", 7, 5, 8, 0)
  self.map:add_camera("right", 7, 5, 248, 0, 0.5, 0.5, 0.25)
  self.map:add_camera("main", 14, 5, 8, 160)

  self.player = { x = 640, y = 640 }

  self.map:camera_from_id("left"):move_to(200, 200)
  self.map:camera_from_id("right"):move_to(800, 200)
  for _, camera in pairs(self.map:get_cameras()) do
    camera.post_draw = function(me, canvas)
        local x, y = me:to_screen(me.x, me.y)
        canvas:rectangle("fill", x - 2, y - 2, 4, 4, 2)
        self.font:write(canvas, me.screen_x + me.screen_width, me.screen_y, tostring(me), "right")
      end
  end
end

function Main:process()
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
  local canvas = Canvas.default()
  canvas:clear()
  self.map:draw(canvas)

  local camera = self.map:camera_from_id("right")
  local x, y = camera:to_screen(self.player.x, self.player.y)
  canvas:rectangle("fill", x - 2, y - 2, 4, 4, 1)

  self.font:write(canvas, 0, 0, string.format("FPS: %d", System.fps()))
end

return Main