pico-8 cartridge // http://www.pico-8.com
version 8
__lua__

-- Small test framework
do local ctx, fail, total = "", 0, 0
   function fixture(name)
       ctx = name
       a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z =
       0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
   end
   function test_equal(x, y)
       total = total + 1
       if x ~= y then
           print(ctx.." failed: '"..x.."' != '"..y.."'")
           fail = fail + 1
       end
   end
   function summary() print("\n"..total.." tests - "..(total - fail).." passed, "..fail.." failed.") end
end

--
-- Check that != works properly
--

fixture("t1.01")
test_equal(1 != 1, false)

fixture("t1.02")
test_equal(1 != 2, true)

fixture("t1.03")
test_equal((1 != 2) != (3 != 4), false)

fixture("t1.04")
test_equal("!=", "!".."=")

--
-- Check several variations of the += syntactic sugar
--

fixture("t2.01")
x += 1
test_equal(x, 1)

fixture("t2.02")
x +=1
test_equal(x, 1)

fixture("t2.03")
x+= 1
test_equal(x, 1)

fixture("t2.04")
x+=1 x+=1
test_equal(x, 2)

fixture("t2.05")
x+=1;x+=1
test_equal(x, 2)

fixture("t2.06")
x+=(1)x+=1
test_equal(x, 2)

-- this is not pretty but should be legal because Lua parses numbers
-- until the last legal digit/character
fixture("t2.07")
x+=1x+=1
test_equal(x, 2)

fixture("t2.08")
x+=1-- intrusive comment
test_equal(x, 1)

fixture("t2.09")
x-- more
+=-- intrusive
1-- comments
test_equal(x, 1)

-- nested reassignments; will confuse most regex-based methods that
-- attempt to convert PICO-8 code to standard Lua
fixture("t2.10")
x+=(function(x)x+=1 return x end)(2)
test_equal(x, 3)

fixture("t2.11") -- nested reassignments and ugliness!
x+=1x+=(function(x)x+=1x+=x return x end)(1)
test_equal(x, 5)

--
-- TODO: Check several variations of if/then or if without then
--

--fixture("t3.01")
--if (x == 0) then x=1 end
--if (x == 1) x=2
--if (x != 2) then x=2 else x=3 end
--if (x != 3) x=3 else x=4
--test_equal(x, 4)

summary()

