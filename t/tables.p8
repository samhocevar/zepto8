pico-8 cartridge // http://www.pico-8.com
version 8
__lua__
-- zepto-8 conformance tests
-- for lua syntax extensions

-- small test framework
do local sec, sn, ctx, cn = "", 0, "", 0
   local fail, total, idx = 0, 0, 0
   function section(name)
       sec = name
       sn += 1
   end
   function fixture(name)
       ctx = name
       cn += 1
       idx = 0
       a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z =
       0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
   end
   function test_equal(x, y)
       total += 1
       idx += 1
       if x ~= y then
           printh('section '..sec..':')
           printh(ctx.." #"..idx.." failed: '"..tostr(x).."' != '"..tostr(y).."'")
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

fixture 'add at 1'
    t={1,2,3}
    add(t,4,1)
    test_equal(t[1],4)
    test_equal(t[2],1)

fixture 'add at -10'
    t={1,2,3}
    add(t,4,-10)
    test_equal(t[1],4)
    test_equal(t[2],1)

fixture 'add at #t'
    t={1,2,3}
    add(t,4,#t)
    test_equal(t[3],4)
    test_equal(t[4],3)

fixture 'add at #t+1'
    t={1,2,3}
    add(t,4,#t+1)
    test_equal(t[3],3)
    test_equal(t[4],4)

fixture 'add at #t+10'
    t={1,2,3}
    add(t,4,#t+10)
    test_equal(t[3],3)
    test_equal(t[4],4)

--
-- Check that all() works properly
--

section 'all()'

fixture 'empty'
    t={}
    s=''
    for n in all(t) do s = s..tostr(n) end
    test_equal(s,'')

fixture 'simple'
    t={1,2}
    s=''
    for n in all(t) do s = s..tostr(n) end
    test_equal(s,'12')

fixture 'dupes'
    t={1,1,2,2}
    s=''
    for n in all(t) do s = s..tostr(n) end
    test_equal(s,'1122')

fixture 'holes'
    t={1,nil,2,nil,nil,3}
    s=''
    for n in all(t) do s = s..tostr(n) end
    test_equal(s,'123')

--
-- Check that foreach() works properly
--

section 'foreach()'

fixture 'simple'
    t={1,2,3}
    v={}
    foreach(t, function(x) add(v,x) end)
    test_equal(v[1],1)
    test_equal(v[2],2)
    test_equal(v[3],3)
    test_equal(v[4],nil)

fixture 'inner add'
    t={1,2,3}
    v={}
    foreach(t, function(x) add(v,x) if(x==2) then add(t,4) end end)
    test_equal(v[1],1)
    test_equal(v[2],2)
    test_equal(v[3],3)
    test_equal(v[4],4)
    test_equal(v[5],nil)

fixture 'inner del'
    t={1,2,3}
    v={}
    foreach(t, function(x) add(v,x) if(x==2) then del(t,3) end end)
    test_equal(v[1],1)
    test_equal(v[2],2)
    test_equal(v[3],nil)

fixture 'inner del+add'
    t={1,2,3}
    v={}
    foreach(t, function(x) add(v,x) if(x==2) then del(t,3) add(t,4) end end)
    test_equal(v[1],1)
    test_equal(v[2],2)
    test_equal(v[3],4)
    test_equal(v[4],nil)

fixture 'inner del+del+add'
    t={1,2,3}
    v={}
    foreach(t, function(x) add(v,x) if x==2 then del(t,2) del(t,3) add(t,4) end end)
    test_equal(v[1],1)
    test_equal(v[2],2)
    test_equal(v[3],4)
    test_equal(v[4],nil)

fixture 'inner del+del+del+add'
    t={1,2,3}
    v={}
    foreach(t, function(x) add(v,x) if x==2 then del(t,1) del(t,2) del(t,3) add(t,4) end end)
    test_equal(v[1],1)
    test_equal(v[2],2)
    test_equal(v[3],nil)

fixture 'nested foreach'
    t={1,2}
    v={}
    foreach(t, function(x) foreach(t, function(y) add(v,x*100+y) end) end)
    test_equal(v[1],101)
    test_equal(v[2],102)
    test_equal(v[3],201)
    test_equal(v[4],202)
    test_equal(v[5],nil)

fixture 'nested foreach delete'
    t={1,2,3}
    v={}
    foreach(t, function(x) if x==2 then foreach(t, function(y) del(t,y) end) end add(v,x) end)
    test_equal(t[1],nil)
    test_equal(v[1],1)
    test_equal(v[2],2)
    test_equal(v[3],nil)

--
-- print report
--

summary()

