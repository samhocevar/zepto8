pico-8 cartridge // http://www.pico-8.com
version 8
__lua__

-- Check that != does not get replaced in strings
s1 = "!="
s2 = "!".."="
check_equal(s1, s2)

-- Check several variations of the += syntactic sugar
x=0
if (x != 2) then
    x += 1
    x +=2
    x+= 4
    x+=8 x+=16
    x+=32;x+=64
    x+=(128)x+=256
    x+=512--lol
end
check_equal(x, 1023)

-- Check several variations of if/then or if without then
x=0
--if (x == 0) then x=1 end
--if (x == 1) x=2
--if (x != 2) then x=2 else x=3 end
--if (x != 3) x=3 else x=4
check_equal(x, 4)

