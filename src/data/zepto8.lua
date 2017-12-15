--
--  ZEPTO-8 — Fantasy console emulator
--
--  Copyright © 2016—2017 Sam Hocevar <sam@hocevar.net>
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
do
    -- According to https://gist.github.com/josefnpat/bfe4aaa5bbb44f572cd0 :
    --  coroutine.[create|resume|status|yield]() was removed in 0.1.3 but added
    --  in 0.1.6 as coroutine(), cocreate(), coresume(), costatus() and yield()
    --  respectively.
    cocreate = coroutine.create
    coresume = coroutine.resume
    costatus = coroutine.status
    yield = coroutine.yield

    -- use closure so that we don’t need “table” later
    local insert = table.insert
    local remove = table.remove

    count = function(a) return a ~= nil and #a or 0 end
    add = function(a, x) if a ~= nil then insert(a, x) end return x end
    sub = string.sub

    foreach = function(a, f)
        if a ~= nil then for k, v in ipairs(a) do f(v) end end
    end

    all = function(a)
        local i, n = 0, a ~= nil and #a or 0
        return function() i = i + 1 if i <= n then return a[i] end end
    end

    del = function(a, v)
        if a ~= nil then
            for k, v2 in ipairs(a) do
                if v == v2 then remove(a, k) return end
            end
        end
    end

    -- PICO-8 documentation: t() aliased to time()
    t = _z8.time

    -- Use the new peek4() and poke4() functions
    dget = function(n)
        n = tonumber(n)
        return n >= 0 and n < 64 and peek4(0x5e00 + 4 * n) or 0
    end

    dset = function(n, x)
        n = tonumber(n)
        if n >= 0 and n < 64 then poke4(0x5e00 + 4 * n, x) end
    end

    local match = string.match

    cartdata = function(s)
        if _z8.__cartdata() then
            print('cartdata() can only be called once')
            abort()
            return false
        end
        -- PICO-8 documentation: id is a string up to 64 characters long
        if #s == 0 or #s > 64 then
            print('cart data id too long')
            abort()
            return false
        end
        -- PICO-8 documentation: legal characters are a..z, 0..9 and underscore (_)
        if match(s, '[^abcdefghijklmnopqrstuvwxyz0123456789_]') then
            print('cart data id: bad char')
            abort()
            return false
        end
        return _z8.__cartdata(s)
    end

    -- All flip() does for now is yield so that the C++ VM gets a chance
    -- to draw something even if Lua is in an infinite loop
    flip = function()
        yield()
    end

    -- Backward compatibility for old PICO-8 versions
    mapdraw = _z8.map
end


--
-- Make all our C++ static methods global then hide _G
-- According to https://gist.github.com/josefnpat/bfe4aaa5bbb44f572cd0 :
--  _G global table has been removed.
--
for k, v in pairs(_z8) do _G[k] = string.sub(k, 1, 2) ~= '__' and v or nil end
_G = nil


--
-- Hide these modules, they should not be accessible
--
table = nil
debug = nil
string = nil
io = nil
coroutine = nil


--
-- Utility functions
--
_z8.reset_drawstate = function()
    -- From the PICO-8 documentation:
    -- “The draw state is reset each time a program is run. This is equivalent to calling:
    -- clip() camera() pal() color()”
    --
    -- Note from Sam: this should probably be color(6) instead.
    clip() camera() pal() color(6) fillp()
end

_z8.reset_cartdata = function()
    _z8.__cartdata(nil)
end

_z8.run = function(cart_code)
    _z8.loop = cocreate(function()
        local do_frame = true

        -- First reload cart into memory
        memset(0, 0, 0x8000)
        reload()

        _z8.reset_drawstate()
        _z8.reset_cartdata()

        -- Load cart
        cart_code()

        -- Initialise if available
        if _init ~= nil then _init() end

        -- Execute the user functions
        while true do
            if _update60 ~= nil then
                _update_buttons()
                _update60()
            elseif _update ~= nil then
                if do_frame then
                    _update_buttons()
                    _update()
                end
                do_frame = not do_frame
            end
            if _draw ~= nil and do_frame then
                _draw()
            end
            yield()
        end
    end)
end

_z8.tick = function()
    if costatus(_z8.loop) == "dead" then
        return
    end
    ret, err = coresume(_z8.loop)
    if not ret then
        printh(err)
    end
end


--
-- Initialise the VM
--
srand(0)


--
-- Splash sequence
--
_z8.loop = cocreate(function()
    _z8.reset_drawstate()

    local steps =
    {
        [1] =  function() for i=2,127,8 do for j=0,127 do pset(i,j,rnd()*4+j/40) end end end,
        [7] =  function() for i=0,127,4 do for j=0,127,2 do pset(i,j,(i+j)/8%8+6) end end end,
        [12] = function() for i=2,127,4 do for j=0,127,3 do pset(i,j,rnd()*4+10) end end end,
        [17] = function() for i=1,127,2 do for j=0,127 do pset(i,j,pget(i+1,j)) end end end,
        [22] = function() for j=0,31 do memset(0x6040+j*256,0,192) end end,
        [27] = cls,
        [36] = function() color(7) print("\n\x9a\x9b\x9c\x9d\x9e\x9f")
                           local a = {0,0,12,0,0,0,13,7,11,0,14,7,7,7,10,0,15,7,9,0,0,0,8,0,0}
                           for j=0,#a-1 do pset(41+j%5,2+j/5,a[j+1]) end end,
        [45] = function() color(6) print("\nzepto-8 0.0.0 beta") end,
        [50] = function() print("(c) 2016-17 sam hocevar et al.\n\n") end,
    }

    for step=0,60 do if steps[step] then steps[step]() end flip() end
end)

