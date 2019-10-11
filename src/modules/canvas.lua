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

return Canvas
