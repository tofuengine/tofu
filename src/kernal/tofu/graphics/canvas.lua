--[[
                ___________________  _______________ ___
                \__    ___/\_____  \ \_   _____/    |   \
                  |    |    /   |   \ |    __) |    |   /
                  |    |   /    |    \|     \  |    |  /
                  |____|   \_______  /\___  /  |______/
                                   \/     \/
        ___________ _______    ________.___ _______  ___________
        \_   _____/ \      \  /  _____/|   |\      \ \_   _____/
         |    __)_  /   |   \/   \  ___|   |/   |   \ |    __)_
         |        \/    |    \    \_\  \   /    |    \|        \
        /_______  /\____|__  /\______  /___\____|__  /_______  /
                \/         \/        \/            \/        \

MIT License

Copyright (c) 2019-2026 Marco Lizza

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

local Image <const> = require("tofu.graphics.image")
local State <const> = require("tofu.graphics.state")

local Canvas <const> = {}

local _default = nil

local _states <const> = setmetatable({}, { __mode = "k" }) -- Weak table, don't keep objects alive

function Canvas.from_image(...)
  local args <const> = { ... }
  if #args == 1 then -- name
    return Canvas.new(Image.new(args[1]))
  elseif #args == 2 then -- width, height
    return Canvas.new(Image.new(args[1], args[2]))
  else
    error("invalid arguments for `from_image` method", 2)
  end
end

function Canvas.default()
  if not _default then
    local image <const> = Image.new() -- Get a reference to the VRAM as an `Image`.
    _default = Canvas.new(image)
  end
  return _default
end

function Canvas:state()
  local state = _states[self]
  if not state then
    state = State.from_canvas(self)
    _states[self] = state
  end
  return state
end

function Canvas:square(mode, x, y, size, index)
  self:rectangle(mode, x, y, size, size, index)
end

-- Only `x`, `y`, `font`, and `text` are required. All the other arguments are optional.
--
-- From the [reference manual](https://www.lua.org/pil/5.1.html)
-- << [...] A function call that is not the last element in the list always produces one
-- result [...] When a function call is the last (or the only) argument to another call,
-- all results from the first call go as arguments. >>
function Canvas:write(x, y, font, text, h_align, v_align, scale_x, scale_y)
  local width <const>, height <const> = font:size(text, scale_x or 1.0, scale_y or scale_x or 1.0)

  if h_align == "center" then
    x = x - tonumber(width * 0.5)
  elseif h_align == "right" then
    x = x - width
  end
  if v_align == "middle" then
    y = y - tonumber(height * 0.5)
  elseif v_align == "bottom" then
    y = y - height
  end

  -- Scaling is not the usual scenario, so be invert the checking order to
  -- avoid the extra checks when not needed (we will enter the first branch
  -- most of the times).
  if not scale_x then
    return self:text(x, y, font, text)
  elseif not scale_y then
    return self:text(x, y, font, text, scale_x)
  else
    return self:text(x, y, font, text, scale_x, scale_y)
  end
end

return Canvas
