local Class = {}

function Class.define(model)
  local proto = {}
  -- If a base class is defined, the copy all the functions.
  --
  -- This is an instant snapshot, any new field defined runtime in the base
  -- class won't be visible in the derived class.
  if model then
    Class.implement(proto, model)
  end
  -- This is the standard way in Lua to implement classes.
  proto.__index = proto
  proto.new = function(...)
      local self = setmetatable({}, proto)
      if self.__ctor then
        self:__ctor(...)
      end
      return self
    end
  return proto
end

function Class.implement(proto, model)
  for key, value in pairs(model) do
    if type(value) == 'function' then
      proto[key] = value
    end
  end
end

return Class
