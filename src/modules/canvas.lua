local Canvas = {}

function Canvas.polyline(vertices, index)
  local count = #vertices
  if count < 4 then -- At least two vertices are required for a line!
    return
  end
  local x0, y0 = vertices[1], vertices[2]
  for i = 3, count, 2 do
    local x1, y1 = vertices[i], vertices[i + 1]
    Canvas.line(x0, y0, x1, y1, index)
    x0, y0 = x1, y1
  end
end

function Canvas.square(mode, x, y, size, index)
  Canvas.rectangle(mode, x, y, size, size, index)
end

local function plotpoints(cx, cy, x, y, index)
  Canvas.point(cx - x, cy + y, index)
  Canvas.point(cx - y, cy - x, index)
  Canvas.point(cx + x, cy - y, index)
  Canvas.point(cx + y, cy + x, index)
end

local function plotlines(cx, cy, x, y, index)
  local w = math.abs(2 * x) + 1
  Canvas.hline(cx + x, cy + y, w, index)
  Canvas.hline(cx + x, cy - y, w, index)
end

-- https://gist.github.com/bert/1085538/f288057c6fb08b61bf97e999d9237f6e04e4f444
function Canvas.circle(mode, center_x, center_y, radius, index)
  local plot = mode == "fill" and plotlines or plotpoints
  local r, cx, cy = math.floor(radius), math.floor(center_x), math.floor(center_y)
  local x, y, err = -r, 0, 2 - 2 * r
  repeat
    plot(cx, cy, x, y, index)
    r = err
    if r <= y then
      y = y + 1
      err = err + y * 2 + 1
    end
    if r > x or err > y then
      x = x + 1
      err = err + x * 2 + 1
    end
  until x == 0
end

return Canvas
