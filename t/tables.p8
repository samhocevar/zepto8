pico-8 cartridge // http://www.pico-8.com
version 8
__lua__
-- zepto-8 conformance tests
-- for lua syntax extensions

-- small test framework
do local sec, sn, ctx, cn = "", 0, "", 0
   local fail, total = 0, 0
   function section(name)
       sec = name
       sn += 1
   end
   function fixture(name)
       ctx = name
       cn += 1
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
   function summary()
       print("\n"..total.." tests - "..(total - fail).." passed, "..fail.." failed.")
   end
end

--
-- Check that # works properly
--

section 'operator #'

fixture 'empty'
    t={}
    test_equal(#t,0)

fixture 'nil in table'
    t={nil}
    test_equal(#t,0)

fixture 'one'
    t={1}
    test_equal(#t,1)

--
-- Check that count() works properly
--

section 'count()'

fixture 'empty'
    t={}
    test_equal(count(t),0)

fixture 'empty string'
    t=""
    test_equal(count(t),0)

fixture 'nil in table'
    t={nil}
    test_equal(count(t),0)

fixture 'one'
    t={1}
    test_equal(count(t),1)

fixture 'three'
    t={1,2,3}
    test_equal(count(t),3)

fixture 'three minus one'
    t={1,2,3}
    t[3]=nil
    test_equal(count(t),2)

fixture 'three plus one'
    t={1,2,3}
    t[4]=4
    test_equal(count(t),4)

fixture 'three plus sparse'
    t={1,2,3}
    t[5]=5
    test_equal(count(t),3)

fixture 'three with hole'
    t={1,2,3}
    t[2]=nil
    test_equal(count(t),2)

--
-- print report
--

summary()

