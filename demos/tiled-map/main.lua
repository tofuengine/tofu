local System = require("tofu.core").System
local Canvas = require("tofu.graphics").Canvas
local Font = require("tofu.graphics").Font
local Input = require("tofu.events").Input
local Class = require("tofu.util").Class

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
--]]--

function Main:__ctor()
  Canvas.palette("gameboy")

  self.font = Font.default(0, 3)
  self.map = Map.new("assets/world.map")

  self.map:add_camera("left", 7, 5, 8, 0)
  self.map:add_camera("right", 7, 5, 248, 0)
  self.map:add_camera("main", 14, 5, 8, 160)

  self.player = { x = 640, y = 640 }

  self.map:camera_from_id("left"):move_to(200, 200)
  self.map:camera_from_id("right"):move_to(800, 200)
end

function Main:input()
  self.dx = 0
  self.dy = 0
  if Input.is_key_down(Input.LEFT) then
    self.dx = self.dx - 1
  end
  if Input.is_key_down(Input.RIGHT) then
    self.dx = self.dx + 1
  end
  if Input.is_key_down(Input.UP) then
    self.dy = self.dy - 1
  end
  if Input.is_key_down(Input.DOWN) then
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
  local dx = self.dx * CAMERA_SPEED * delta_time
  local dy = self.dy * CAMERA_SPEED * delta_time
  if dx ~= 0.0 or dy ~= 0.0 then
    self.player.x, self.player.y = camera:bound(self.player.x + dx, self.player.y + dy)
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

  self.font:write(self.map:to_string(), Canvas.width(), 0, "right")
  self.font:write(string.format("FPS: %d", System.fps()), 0, 0, "left")
end

return Main