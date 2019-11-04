local Math = {}

function Math.lerp(a, b, x)
  -- More precise than `a + (b - a) * x`.
  return a * (1.0 - x) + b * x
end

function Math.sine_wave(period, t)
  local t_over_p = t / period
  return math.sin(t_over_p * 2 * math.pi)
end

function Math.square_wave(period, t)
  local t_over_p = t / period
  return 2.0 * (2.0 * math.floor(t_over_p) - math.floor(2 * t_over_p)) + 1.0
end

function Math.triangle_wave(period, t)
  local t_over_p = t / period
  return 2.0 * math.abs(2.0 * (t_over_p - math.floor(t_over_p + 0.5))) - 1.0
end

function Math.sawtooth_wave(period, t)
  local t_over_p = t / period
  return 2.0 * (t_over_p - math.floor(0.5 + t_over_p))
end

return Math
