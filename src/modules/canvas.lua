local Canvas = {}

function Canvas.square(mode, x, y, size, index)
  Canvas.rectangle(mode, x, y, size, size, index)
end

return Canvas
