local Font = {}

--Font.__index = Font

-- ! can't declare as `Font.default = function(...)`
function Font.default(background_color, foreground_color)
  return Font.new("5x8", 0, 0, background_color, foreground_color)
end

return Font
