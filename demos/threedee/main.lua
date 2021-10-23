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
local Canvas = require("tofu.graphics").Canvas
local Display = require("tofu.graphics").Display
local Font = require("tofu.graphics").Font
local Palette = require("tofu.graphics").Palette

local Background = require("lib/background")
local Camera = require("lib/camera")
local Player = require("lib/player")
local Scene = require("lib/scene")

local config = require("config")

local Main = Class.define()

local CAMERA_FIELD_OF_VIEW <const> = config.camera.field_of_view or math.pi / 4
local CAMERA_NEAR <const> = config.camera.near or 1
local CAMERA_FAR <const> = config.camera.far or 1000

function Main:__ctor()
  local palette <const> = Palette.new(config.display.palette)
  Display.palette(palette)

  local canvas <const> = Canvas.default()
  local width <const>, height <const> = canvas:size()
  canvas:transparent({ [0] = false, [63] = true })

  self.player = Player.new()
  self.camera = Camera.new(CAMERA_FIELD_OF_VIEW, width, height, CAMERA_NEAR, CAMERA_FAR)
  self.scene = Scene.new(self.camera, palette, 63)
  self.background = Background.new(self.camera, 59)
  self.font = Font.default(63, 11)

  self.running = true
end

function Main:process()
  local camera <const> = self.camera

  if Input.is_pressed("y") then
    local field_of_view = math.min(camera.field_of_view + math.pi / 32, math.pi)
    camera:set_field_of_view(field_of_view)
  elseif Input.is_pressed("x") then
    local field_of_view = math.max(camera.field_of_view - math.pi / 32, 0)
    camera:set_field_of_view(field_of_view)
  end
  if Input.is_pressed("b") then
    local far = math.min(camera.far + 50.0, 5000.0)
    camera:set_clipping_planes(camera.near, far)
  elseif Input.is_pressed("a") then
    local far = math.max(camera.far - 50.0, 0)
    camera:set_clipping_planes(camera.near, far)
  end
  if Input.is_pressed("start") then
    self.player.pause = not self.player.pause
  end
  if Input.is_pressed("select") then
    self.running = not self.running
  end

  self.player:process()
end

function Main:update(delta_time)
  if not self.running then
    return
  end

  local player = self.player
  local camera = self.camera

  player:update(delta_time)
  camera:move(player:position())

  -- Rebuild sky and ground (copperlist) program.
  self.background:update(delta_time)

  self.scene:update(delta_time)
end

function Main:render(_)
  local camera <const> = self.camera

  local canvas = Canvas.default()
  canvas:clear(59)

  self.background:render(canvas)
  self.scene:render(canvas)

  self.font:write(canvas, 0, 0, string.format("FPS: %d", System.fps()))
  self.font:write(canvas, 0, 10, string.format("%.3f %.3f %.3f", camera.field_of_view, camera.near, camera.far))
end

return Main
