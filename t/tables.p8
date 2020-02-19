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
           printh('section '..sec..':')
           printh(ctx.." failed: '"..tostr(x).."' != '"..tostr(y).."'")
           fail = fail + 1
       end
   end
   function summary()
       printh("\n"..total.." tests - "..(total - fail).." passed, "..fail.." failed.")
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
-- Check that add() works properly
--

section 'add()'

fixture 'add to empty'
    t={}
    add(t,1)
    test_equal(t[1],1)

fixture 'return value'
    x=add({},1)
    test_equal(x,1)

fixture 'return nil'
    x=add(nil,1)
    test_equal(x,nil)

fixture 'add to 1,2'
    t={1,2}
    add(t, 3) -- {1,2,3}
    test_equal(t[3],3)

fixture 'add to *,2,3'
    t={[2]=2,[3]=3}
    add(t,4) -- {4,2,3}
    test_equal(t[1],4)

fixture 'hole *,2,3'
    t={1,2,3}
    t[1]=nil
    add(t,4) -- {nil,2,3,4}
    test_equal(t[4],4)

fixture 'hole 1,*,3'
    t={1,2,3}
    t[2]=nil
    add(t,4) -- {1,nil,3,4}
    test_equal(t[4],4)

fixture 'hole 1,2,*'
    t={1,2,3}
    t[3]=nil
    add(t,4) -- {1,2,4}
    test_equal(t[3],4)

--
-- print report
--

summary()

