--[[
  Copyright (c) 2019 Marco Lizza (marco.lizza@gmail.com)

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

local Grid = require("tofu.collections").Grid
local Bank = require("tofu.graphics").Bank
local Canvas = require("tofu.graphics").Canvas
local File = require("tofu.io").File
local Class = require("tofu.util").Class

local Camera = require("lib.camera")

-- https://developer.mozilla.org/en-US/docs/Games/Techniques/Tilemaps/Square_tilemaps_implementation:_Scrolling_maps

local function bound(x, y, aabb)
  return math.min(math.max(x, aabb.x0), aabb.x1), math.min(math.max(y, aabb.y0), aabb.y1)
end

local Map = Class.define()

function Map:__ctor(file)
  self:load_(file)

  self.aabb = {
      x0 = 0,
      y0 = 0,
      x1 = self.grid:width() * self.bank:cell_width() - 1,
      y1 = self.grid:height() * self.bank:cell_height() - 1
    }

  self.cameras = {}
end

function Map:load_(file)
  local content = File.as_string(file)
  local tokens = {}
  for chunk in string.gmatch(content, "[^\n]+") do
    table.insert(tokens, chunk)
  end
  local cells = {}
  for i = 6, #tokens do
    local chunk = tokens[i]
    for cell in string.gmatch(chunk, "[^ ]+") do
      table.insert(cells, tonumber(cell))
    end
  end
  self.bank = Bank.new(tokens[1], tonumber(tokens[2]), tonumber(tokens[3]))
  self.grid = Grid.new(tonumber(tokens[4]), tonumber(tokens[5]), cells)
end

function Map:bound(x, y)
  return bound(x, y, self.aabb)
end

function Map:get_cameras()
  return self.cameras
end

function Map:add_camera(id, columns, rows, screen_x, screen_y, anchor_x, anchor_y) -- last two arguments optional
  self.cameras[id] = Camera.new(id, self.grid, self.bank, columns, rows, screen_x, screen_y, anchor_x, anchor_y)
end

function Map:remove_camera(id)
  self.cameras[id] = nil
end

function Map:camera_from_id(id) -- last two arguments are optional
  return self.cameras[id]
end

function Map:update(delta_time)
  for _, camera in pairs(self.cameras) do
    camera:update(delta_time)
  end
end

function Map:draw()
  Canvas.push()
  for _, camera in pairs(self.cameras) do
    camera:pre_draw()
    camera:draw()
    camera:post_draw()
  end
  Canvas.pop()
end

function Map:__tostring()
  local s = nil
  for _, camera in pairs(self.cameras) do
    if not s then
      s = ""
    else
      s = s .. " "
    end
    s = s .. tostring(camera)
  end
  return s
end

return Map