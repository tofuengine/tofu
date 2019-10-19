local Math = {}

function Math.tri(period, x)
  return period - math.abs(x % (2 * period) - period)
end

function Math.sincos(rotation)
  return rotation, 1
end

function Math.angle_to_rotation(angle)
  return angle
end

return Math
