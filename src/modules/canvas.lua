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

-- That's not a proper filled-triangle drawing, since it is potentially
-- messed up by any previous existent content.
function Canvas.triangle(mode, x0, y0, x1, y1, x2, y2, index)
  Canvas.polyline({ x0, y0, x1, y1, x2, y2, x0, y0 }, index)
  if mode ~= "line" then
    local gx, gy = (x0 + x1 + x2) / 3, (y0 + y1 + y2) / 3
    Canvas.fill(gx, gy, index)
  end
end

function Canvas.rectangle(mode, x, y, width, height, index)
  if mode == "line" then
    local x0 = x
    local y0 = y
    local x1 = x0 + width - 1
    local y1= y0 + height - 1
    Canvas.line(x0, y0, x0, y1, index)
    Canvas.line(x0, y1, x1, y1, index)
    Canvas.line(x1, y1, x1, y0, index)
    Canvas.line(x1, y0, x0, y0, index)
  else
    for i = 0, height - 1 do
      Canvas.hline(x, y + i, width, index)
    end
  end
end

function Canvas.square(mode, x, y, size, index)
  Canvas.rectangle(mode, x, y, size, size, index)
end

function Canvas.circle(mode, cx, cy, radius, index)
  local r, x, y, err = radius, -radius, 0, 2 - 2 * radius
  repeat
    if mode == line then
      Canvas.point(cx - x, cy + y, index)
      Canvas.point(cx - y, cy - x, index)
      Canvas.point(cx + x, cy - y, index)
      Canvas.point(cx + y, cy + x, index)
    else
      local w = math.abs(2 * x)
      Canvas.hline(cx + x, cy + y, w, index)
      Canvas.hline(cx + x, cy - y, w, index)
    end
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
