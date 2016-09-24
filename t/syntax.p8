pico-8 cartridge // http://www.pico-8.com
version 8
__lua__

-- Small test framework
do local fail, total = 0, 0
   function check_equal(name, x, y)
       total = total + 1
       if x ~= y then
           print(name.." failed: '"..x.."' != '"..y.."'")
           fail = fail + 1
       end
   end
   function check_summary() print(total.." tests: "..(total - fail).." passed, "..fail.." failed.") end
end

-- Check that != works but != does not get replaced in strings
check_equal("t1.01", 1 != 1, false)
check_equal("t1.02", 1 != 2, true)
x = (1 != 2) != (1 != 2)
check_equal("t1.03", x, false)
s1 = "!="
s2 = "!".."="
check_equal("t1.04", s1, s2)

-- Check several variations of the += syntactic sugar
x=0
x += 1
check_equal("t2.01", x, 1)
x +=1
check_equal("t2.02", x, 2)
x+= 1
check_equal("t2.03", x, 3)
--x+=1 x+=1
--check_equal("t2.04", x, 5)
--x+=1;x+=1
--check_equal("t2.05", x, 7)
--x+=(1)x+=1
--check_equal("t2.06", x, 9)
--x+=1--lol
--check_equal("t2.07", x, 10)

-- Check several variations of if/then or if without then
x=0
if (x == 0) then x=1 end
--if (x == 1) x=2
if (x != 2) then x=2 else x=3 end
--if (x != 3) x=3 else x=4
check_equal("t3.01", x, 4)

check_summary()

