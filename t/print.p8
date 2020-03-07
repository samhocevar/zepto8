pico-8 cartridge // http://www.pico-8.com
version 8
__lua__
-- zepto-8 conformance tests
-- for print()

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

-- small crc for the screen
function crc()
  local x = 0
  for i=0x6000,0x7fff,4 do
    local p=peek4(i)
    x+=p*0xedb8.4e73+rotl(p,16)*0xbe36.7d8f+bxor(rotl(x,7),0xdead.beef)
  end
  return x
end

--
-- t1. check that print() changes the screen
--

fixture "t1.01"
    cls() a=crc()
    print('x')
    test_equal(crc() == a, false)

--
-- t2. print() for printable types
--

fixture "t2.01"
    cls() print('false') a=crc() cls()
    print(false)
    test_equal(crc(), a)

fixture "t2.02"
    cls() print('true') a=crc() cls()
    print(true)
    test_equal(crc(), a)

fixture "t2.03"
    cls() print('1') a=crc() cls()
    print(1)
    test_equal(crc(), a)

--
-- t3. print() for non-printable types
--

fixture "t3.01"
    cls() print('[nil]') a=crc() cls()
    print(nil)
    test_equal(crc(), a)

fixture "t3.02"
    cls() print('[table]') a=crc() cls()
    print({})
    test_equal(crc(), a)

fixture "t3.03"
    cls() print('[function]') a=crc() cls()
    print(cos)
    test_equal(crc(), a)

fixture "t3.04"
    cls() print('[thread]') a=crc() cls()
    print(cocreate(cos))
    test_equal(crc(), a)

--
-- print report
--

summary()
