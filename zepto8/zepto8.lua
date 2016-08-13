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


-- Make all our C++ static methods global
for k, v in pairs(_z8) do _G[k] = v end


-- This is easily implemented in Lua
function foreach(t, f)
    for k, v in ipairs(t) do f(v) end
end

