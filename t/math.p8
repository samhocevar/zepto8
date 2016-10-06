pico-8 cartridge // http://www.pico-8.com
version 8
__lua__
-- zepto-8 conformance tests
-- for lua syntax extensions

-- small test framework
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
-- t1. check that shl works properly
--

fixture "t1.01"
x = shl(1, 1)
test_equal(x, 2)

fixture "t1.02"
x = shl(1, 0)
test_equal(x, 1)

fixture "t1.03"
x = shl(1, 14)
test_equal(x, 16384)

fixture "t1.04"
x = shl(1, 15)
test_equal(x, -32768)

fixture "t1.05"
x = shl(1, 16)
test_equal(x, 0)

fixture "t1.06"
x = shl(1.125, 1)
test_equal(x, 2.25)

fixture "t1.07"
x = shl(16384, 1)
test_equal(x, -32768)

fixture "t1.08"
x = shl(-32768, 1)
test_equal(x, 0)

fixture "t1.09"
x = shl(-32767.5, 1)
test_equal(x, 1)

fixture "t1.10"
x = shl(-1, 1)
test_equal(x, -2)

fixture "t1.11"
x = shl(-1, 15)
test_equal(x, -32768)

fixture "t1.12"
x = shl(-1, 16)
test_equal(x, 0)

fixture "t1.13"
x = shl(-1, 32)
test_equal(x, -1)

fixture "t1.14"
x = shl(-1, 33)
test_equal(x, -2)

--
-- t2. check that shr works properly
--

fixture "t2.01"
x = shr(2, 1)
test_equal(x, 1)

fixture "t2.02"
x = shr(-2, 1)
test_equal(x, -1)

--
-- print report
--

summary()

