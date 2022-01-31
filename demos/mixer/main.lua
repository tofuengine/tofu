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

local Class = require("tofu.core.class")
local System = require("tofu.core.system")
local Input = require("tofu.events.input")
local Canvas = require("tofu.graphics.canvas")
local Display = require("tofu.graphics.display")
local Font = require("tofu.graphics.font")
local Palette = require("tofu.graphics.palette")
local Storage = require("tofu.io.storage")
local Source = require("tofu.sound.source")

local Main = Class.define()

local SOURCES <const> = {
  { name = "assets/flac/1ch-22050-16.flac", type = "sample" },
  { name = "assets/flac/2ch-48000-16.flac", type = "music" }
}

function Main:__ctor()
  Display.palette(Palette.default("pico-8"))

--  warn("@on")
--  warn("Hello,", "Again")

  self.font = Font.default(0, 15)

  self.sources = {}
  for _, source in ipairs(SOURCES) do
    local instance = Source.new(source.name, source.type)
    table.insert(self.sources, { instance = instance, pan = 0.0, balance = 0.0 })
  end
  Storage.scan(function(name)
    if name:find("^assets/mods/") then
      local instance = Source.new(name, "module")
      table.insert(self.sources, { instance = instance, pan = 0.0, balance = 0.0 })
    end
  end)

  self.sources[2].instance:looped(true)
  self.sources[3].instance:looped(true)
  --self.sources[3].instance:play()

  self.property = 1
  self.current = 1
end

local PROPERTIES <const> = { 'play', 'stop', 'resume', 'gain', 'pan', 'balance', 'mix' }

function Main:process()
  if Input.is_pressed("a") then
    local source = self.sources[self.current]
    if self.property == 1 then
      source.instance:play()
    elseif self.property == 2 then
      source.instance:stop()
    elseif self.property == 3 then
      source.instance:resume()
    elseif self.property == 7 then
      local ll, lr, rl, rr = source.instance:mix()
      source.instance:mix(lr, ll, rr, rl)
    end
  elseif Input.is_pressed("x") then
    local source = self.sources[self.current]
    if self.property == 4 then
      local gain = source.instance:gain()
      source.instance:gain(gain - 0.05)
    elseif self.property == 5 then
      source.pan = math.max(-1.0, source.pan - 0.05)
      source.instance:pan(source.pan)
    elseif self.property == 6 then
      source.balance = math.max(-1.0, source.balance - 0.05)
      source.instance:balance(source.balance)
    end
  elseif Input.is_pressed("y") then
    local source = self.sources[self.current]
    if self.property == 4 then
      local gain = source.instance:gain()
      source.instance:gain(gain + 0.05)
    elseif self.property == 5 then
      source.pan = math.min(1.0, source.pan + 0.05)
      source.instance:pan(source.pan)
    elseif self.property == 6 then
      source.balance = math.min(1.0, source.balance + 0.05)
      source.instance:balance(source.balance)
    end
  elseif Input.is_pressed("up") then
    self.current = math.max(self.current - 1, 1)
  elseif Input.is_pressed("down") then
    self.current = math.min(self.current + 1, #self.sources)
  elseif Input.is_pressed("left") then
    self.property = math.max(self.property - 1, 1)
  elseif Input.is_pressed("right") then
    self.property = math.min(self.property + 1, #PROPERTIES)
  end
end

function Main:update(_)
end

function Main:render(_)
  local canvas = Canvas.default()
  canvas:clear(0)

  local width, height = canvas:size()

  local x, y = 0, 0
  for index, source in ipairs(self.sources) do
      local text = string.format("%s%s %.3f %.3f",
        self.current == index and "*" or " ",
        source.instance:is_playing() and "!" or " ",
        source.pan, source.instance:gain())
    local _, th = self.font:write(canvas, x, y, text)
    y = y + th
  end
  self.font:write(canvas, x, y, PROPERTIES[self.property])
  self.font:write(canvas, width, height, System.fps(), "right", "bottom")
end

return Main
