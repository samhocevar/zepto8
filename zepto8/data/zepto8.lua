--
--  ZEPTO-8 — Fantasy console emulator
--
--  Copyright © 2016 Sam Hocevar <sam@hocevar.net>
--
--  This program is free software. It comes without any warranty, to
--  the extent permitted by applicable law. You can redistribute it
--  and/or modify it under the terms of the Do What the Fuck You Want
--  to Public License, Version 2, as published by the WTFPL Task Force.
--  See http://www.wtfpl.net/ for more details.
--


--
-- Aliases for PICO-8 compatibility
--
count = function(a) return a ~= nil and #a or 0 end
add = table.insert
sub = string.sub

--
-- According to https://gist.github.com/josefnpat/bfe4aaa5bbb44f572cd0 :
--  coroutine.[create|resume|status|yield]() was removed in 0.1.3 but added
--  in 0.1.6 as coroutine(), cocreate(), coresume(), costatus() and yield()
--  respectively.
--
cocreate = coroutine.create
coresume = coroutine.resume
costatus = coroutine.status
yield = coroutine.yield
coroutine.create = nil
coroutine.resume = nil
coroutine.status = nil
coroutine.yield = nil


--
-- Make all our C++ static methods global then hide _G
-- According to https://gist.github.com/josefnpat/bfe4aaa5bbb44f572cd0 :
--  _G global table has been removed.
--
for k, v in pairs(_z8) do _G[k] = v end
_G = nil


--
-- Backward compatibility for old PICO-8 versions
--
mapdraw = map


--
-- These are easily implemented in Lua
--
function foreach(t, f)
    for k, v in ipairs(t) do f(v) end
end

function all(a)
    local i, n = 0, a ~= nil and #a or 0
    return function() i = i + 1 if i <= n then return a[i] end end
end

_z8.remove = table.remove
function del(t, v)
    if t ~= nil then
        for k, v2 in ipairs(t) do
            if v == v2 then _z8.remove(t, k) return end
        end
    end
end


--
-- Hide these modules, they shouldnot be accessible
--
table = nil
string = nil
io = nil
