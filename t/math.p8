pico-8 cartridge // http://www.pico-8.com
version 8
__lua__
-- mATHS tEST sUITE
-- sAM hOCEVAR <SAM@HOCEVAR.NET>

function tostr(x)
    return x == nil and "nil" or ""..x
end

function section(name)
    printh("")
    printh("##########################")
    printh("### "..name)
    printh("")
end


section("atan2")

-- TODO: we want -32768 instead of -32767 below, but it causes PICO-8 to crash
values = { nil, -32767, -1, 0, 1, 32767 }
printh("atan2 = "..atan2())
for i = 1,#values do
    printh("atan2 "..tostr(x).." = "..atan2(x))
    for j = 1,#values do
        x, y = values[i], values[j]
        printh("atan2 "..tostr(x).." "..tostr(y).." = "..atan2(x,y))
    end
end


section("sin/cos")

values = { -32768, -32767, -128, -127, -5, -4, -3, -2, -1, 0 }
steps = { 0, 1/4, 2/4, 3/4 }
printh("sin = "..sin())
printh("cos = "..cos())
printh("sin nil = "..sin(nil))
printh("cos nil = "..cos(nil))
for i = 1,#values do
    for j = 1,#steps do
        x = values[i] + steps[j]
        printh("sin "..tostr(x).." = "..sin(x))
        printh("cos "..tostr(x).." = "..cos(x))
        if (x ~= 0) and (x >= -32767) then
            printh("sin "..tostr(-x).." = "..sin(-x))
            printh("cos "..tostr(-x).." = "..cos(-x))
        end
    end
end


section("boolean ops")

for i = 0,256 do
    -- use flr() to make sure the wrap around at 32768 happens
    x = flr(i * 257)
    for j = 1,256 do
        y = flr(j * 257)
        printh("band "..tostr(x).." "..tostr(y).." = "..band(x, y))
        printh("bor "..tostr(x).." "..tostr(y).." = "..bor(x, y))
        printh("bxor "..tostr(x).." "..tostr(y).." = "..bxor(x, y))
    end
    -- bnot(x) can’t be represented exactly so we compute this value instead
    z = flr((bnot(x) - x) * 256 * 256)
    printh("bnot "..tostr(x).." = "..tostr(flr(-x)).." + "..tostr(z).." / 65536")
end

