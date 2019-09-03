local Canvas = {}

function Canvas.point(x0, y0, color)
  Canvas.points({ x0, y0 }, color)
end

function Canvas.line(x0, y0, x1, y1, color)
  Canvas.polyline({ x0, y0, x1, y1, x0, y0 }, color)
end

function Canvas.triangle(mode, x0, y0, x1, y1, x2, y2, color)
  if mode == "line" then
    Canvas.polyline({ x0, y0, x1, y1, x2, y2, x0, y0 }, color)
  else
    Canvas.strip({ x0, y0, x1, y1, x2, y2 }, color)
  end
end

function Canvas.rectangle(mode, x, y, width, height, color)
  local offset = mode == "line" and 1 or 0
  local x0 = x
  local y0 = y
  local x1 = x0 + width - offset
  local y1= y0 + height - offset
  if mode == "line" then
    Canvas.polyline({ x0, y0, x0, y1, x1, y1, x1, y0, x0, y0 }, color)
  else
    Canvas.strip({ x0, y0, x0, y1, x1, y0, x1, y1 }, color)
  end
end

function Canvas.square(mode, x, y, size, color)
  Canvas.rectangle(mode, x, y, size, size, color)
end

return Canvas
