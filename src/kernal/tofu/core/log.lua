--[[
MIT License

Copyright (c) 2019-2021 Marco Lizza

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
]]--

local Log = {}

function Log.dump(t, spaces)
  spaces = spaces or ""
  for k, v in pairs(t) do
    Log.info(spaces .. k .. " " .. type(v) .. " " .. tostring(v))
    if type(v) == "table" then
      if k ~= "__index" and k ~= "__newindex" then
        Log.dump(v, spaces .. " ")
      end
    end
  end
end

function Log.print_r(value)
  local cache = {}
  local function sub_print_r(t, indent)
    if (cache[tostring(t)]) then
      print(indent.."*"..tostring(t))
    else
      cache[tostring(t)]=true
      if (type(t)=="table") then
        local tLen = #t
        for i = 1, tLen do
          local val = t[i]
          if (type(val)=="table") then
            print(indent.."#["..i.."] => "..tostring(t).." {")
            sub_print_r(val,indent..string.rep(" ",string.len(i)+8))
            print(indent..string.rep(" ",string.len(i)+6).."}")
          elseif (type(val)=="string") then
            print(indent.."#["..i..'] => "'..val..'"')
          else
            print(indent.."#["..i.."] => "..tostring(val))
          end
        end
        for pos,val in pairs(t) do
          if type(pos) ~= "number" or math.floor(pos) ~= pos or (pos < 1 or pos > tLen) then
            if (type(val)=="table") then
              print(indent.."["..pos.."] => "..tostring(t).." {")
              sub_print_r(val,indent..string.rep(" ",string.len(pos)+8))
              print(indent..string.rep(" ",string.len(pos)+6).."}")
            elseif (type(val)=="string") then
              print(indent.."["..pos..'] => "'..val..'"')
            else
              print(indent.."["..pos.."] => "..tostring(val))
            end
          end
        end
      else
        print(indent..tostring(t))
      end
    end
  end

  if (type(value)=="table") then
    print(tostring(value).." {")
    sub_print_r(value,"  ")
    print("}")
  else
    sub_print_r(value,"  ")
  end

  print()
end

return Log
