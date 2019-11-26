local System = require("tofu.core").System
local Canvas = require("tofu.graphics").Canvas
local Font = require("tofu.graphics").Font
local Input = require("tofu.events").Input
local Class = require("tofu.util").Class

local Tilemap = require("lib.tilemap")

local Main = Class.define()

local CAMERA_SPEED = 64.0

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

  self.font = Font.default(2, 3)
  self.map = Tilemap.new("assets/world.map", 15, 10, { ["horizontal"] = "left", ["vertical"] = "top" })

  self.map:move_to(160, 160)
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
  local dx = self.dx * CAMERA_SPEED * delta_time
  local dy = self.dy * CAMERA_SPEED * delta_time
  if dx ~= 0.0 or dy ~= 0.0 then
    self.map:scroll_by(dx, dy)
  end
  self.map:update(delta_time)
end

function Main:render(ratio)
  Canvas.clear()
  self.map:render(ratio)

  self.font:write(string.format("%.0f %0.f %0.f %0.f", self.map.camera_start_column, self.map.camera_start_row,
      self.map.camera_offset_x, self.map.camera_offset_y), Canvas.width(), 0, "right")
  self.font:write(string.format("FPS: %d", System.fps()), 0, 0, "left")
end

return Main