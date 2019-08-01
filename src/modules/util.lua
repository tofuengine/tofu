local Timer = {}

Timer.__index = Timer

function Timer.new(period, repeats, callback)
  return setmetatable({}, Timer)
end

function Timer:reset()
end

function Timer:cancel()
end

return {
    Timer = Timer
  }