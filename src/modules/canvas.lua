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

function Canvas.circle(mode, cx, cy, radius, index)
  local r, x, y, err = radius, -radius, 0, 2 - 2 * radius
  repeat
    if mode == line then
      Canvas.point(cx - x, cy + y, index)
      Canvas.point(cx - y, cy - x, index)
      Canvas.point(cx + x, cy - y, index)
      Canvas.point(cx + y, cy + x, index)
    else
      local w = math.abs(2 * x) + 1
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
