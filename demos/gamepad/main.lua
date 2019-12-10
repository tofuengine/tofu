local Bank = require("tofu.graphics").Bank
local Canvas = require("tofu.graphics").Canvas
local Font = require("tofu.graphics").Font
local Input = require("tofu.events").Input
local Class = require("tofu.util").Class
local Math = require("tofu.core").Math
local System = require("tofu.core").System

local Main = Class.define()

local IDS = {
    Input.A, Input.B, Input.X, Input.Y,
    Input.LT, Input.RT,
    Input.UP, Input.DOWN, Input.LEFT, Input.RIGHT,
    Input.SELECT, Input.START,
    Input.RESET
  }

local INDICES = {
    0, 1, 2, 3, 4, 5,
    12, 13, 14, 15,
    24, 25,
    36
  }

function Main:__ctor()
  Canvas.palette("pico-8")

  Input.key_auto_repeat(Input.Y, 0.5)

  self.bank = Bank.new("assets/sheet.png", 12, 12)
  self.font = Font.default(0, 15)
  self.down = {}
  self.rotation = {}
  self.scale = {}
end

function Main:input()
  for _, id in ipairs(IDS) do
    self.down[id] = Input.is_key_down(id)
    if Input.is_key_pressed(id) then
      self.rotation[id] = Math.SINCOS_PERIOD
      self.scale[id] = 3.0
    end
  end
end

function Main:update(delta_time)
  for _, id in ipairs(IDS) do
    if self.rotation[id] and self.rotation[id] > 0 then
      self.rotation[id] = math.max(0, self.rotation[id] - delta_time * 1024.0)
    end
    if self.scale[id] and self.scale[id] > 1.0 then
      self.scale[id] = math.max(1.0, self.scale[id] - delta_time * 3.0)
    end
  end
end

function Main:render(_)
  local t = System.time()

  Canvas.clear()

  local cw, ch = self.bank:cell_width(), self.bank:cell_height()

  local x, y = (Canvas.width() - #IDS * cw) * 0.5, (Canvas.height() - ch) * 0.5
  for index, id in ipairs(IDS) do
    local dy = math.sin(t * 2.5 + x * 0.5) * ch
    if self.down[id] then
      dy = 0
    end
    -- local r = 0
    -- if self.rotation[id] then
    --   r = self.rotation[id]
    -- end
    local ox, oy = 0, 0
    local s = 1.0
    if self.scale[id] then
      s = self.scale[id]
      ox = (cw * s - cw) * 0.5
      oy = (ch * s - ch) * 0.5
    end
    self.bank:blit(INDICES[index], x - ox, y - oy + dy, s, s)
    x = x + cw
  end

  self.font:write(string.format("FPS: %d", System.fps()), 0, 0, "left")
end

return Main
