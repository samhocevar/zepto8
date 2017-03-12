pico-8 cartridge // http://www.pico-8.com
version 8
__lua__
-- zepton 0.9 beta
-- a game by rez

-- „todo„
-- energy boost (over pyramid)
-- drop bonus (life/shield)
-- add score combo
-- optimize spaceship
-- 

function _init()
	cls()
	cartdata"zepton"
	dbg=dget"8">0 and true or false
	fps={}
	for i=0,23 do fps[i]=0 end
	f,d=128,512 --focale/depth
	nx,nz=40,48 --voxel size
	s=0 --speed
	u,u2,u3,u4,uh=8,16,24,32,4
	uz=d/nz --depth unit
	shp,su,su2,suh={},1,2,0.5
	local n=0
	for k=0,1 do
		for j=0,6 do
			for i=0,6 do
				c=sget(8*k+i,j+16)
				if c!=0 then
					v={
						c=c,
						x=(i-3.5)*su,
						y=-(k+1)*su,
						z=(j-3.5)*su,
						w=su,h=su}
					if c==14 then
						v.w=suh
						v.x+=suh/2
					end
					shp[n]=v
					n+=1
				end
			end
		end
	end
	scr={}  --score
	for i=0,7 do
		scr[i]=dget(i)
	end
	menuitem(1,"new game",
		function() init(0) end)
	menuitem(2,"score",
		function() init(2) end)
	menuitem(3,"reset score",
		function()
			for i=0,7 do
				scr[i]=0
				dset(i,0)
			end
			init(2)
		end)
	menuitem(5,"debug mode",db)
	init(1)
	music(0)
end

function init(m)
	mode=m --o=game/1=menu/2=score
	t,tw,tb=0,2,time()*32 --timer
	fc=0 --frame count
	tv,tz,tn={},{},0 --terrain
	px,py,pz=0,0,0
	sx,sy,sz,se,sh=0,8*u,3.5*uz,100,0
	ix,iy=0,0
	mx,my,mz,mt,ms=0,0,0,false,0
	msl,mn,mp={},0,0 --missile
	bul,bn,bp,bf={},0,0,0 --bullet
	foe,er={},16 --enemy
	lsr,lr={},128 --laser
	smo={} --smoke
	exp={} --explosion
	sta={} --stars
	cs=0   --camera shake
	if mode==0 then
		sc,sn=0,32 --score
		for i=0,sn-1 do
			sta[i]={
				x=flr(rnd(128)),
				y=flr(rnd(48))}
		end
		-------------------init voxel
		for i=0,nz-1 do
			tv[i]=voxel(tn)
			tz[i]=i*uz
			if i%er==0 then
				spawn(i,flr(2+rnd(nx-4)))
			end
			tn+=1
		end
	else ----------------init menu
		sn=512
		local a,x,y,z,zf
		for i=0,sn-1 do
			v={
				z=256/sn*i,
				r=32+rnd(480),a=rnd(1),
				t={}}
			a=v.a+cos(0)/4+(sx+sy)/720
			x=v.r*cos(a)+128-sx
			y=v.r*sin(a)+128-sy
			z=v.z%256
			zf=1/z*32
			x,y=x*zf,y*zf
			for j=0,5 do
				v.t[j]={x=x,y=y,z=z}
			end
			sta[i]=v
		end
		for i=0,7 do
			a=1/6*i
			for j=0,11,2 do
				b=j<6 and 1/28 or 1/24
				sta[sn+i*12+j]={
					x=0,y=0,a=1/6*j+a-b}
				sta[sn+i*12+j+1]={
					x=0,y=0,a=1/6*j+a+b}
			end
		end
	end
end

function voxel(a)
	local az,l=a%nz,{}
	local y,c,bt,by,bw,bh,bc
	local p,vy,bx,ry
	local cl={7,10,9,4,5,3,3,1}
	for j=0,nx-1 do
		----------------------terrain
		y=u2*cos(4/nz*min(a%nz,nz/4))
		y+=u2*cos(3/nx*j) --x
		y+=u2*sin((a-j*4)/nz) --y
		y+=u3*sin((2/nz)*(a+j))
		y=flr(min(u2+y,u4)/uh)*uh
		c=cl[flr(max((u4+y)/8,1))]
		bt,by,bw,bh,bc=0,u4,1,u,0
		----------------------volcano
		vy=-u4
		if y<vy then
			if y>vy-u then
				y=vy+u
				c=6
			else --lava!
				y=vy+u2
				c=8
				by=y-(7+rnd(6)*u)
				bc=6
				bt=1
				bw=u/16+rnd(u2)/32
			end
		end
		------------------------river
		p=27
		if (j==p-1 or j==p+2)
		and y<u4 then
			c=a%4<2 and 6 or 13
		end
		if j>p-1 and j<p+2 then
			y=u4
			c=1
		end
		-----------------------bridge
		bx=13
		ry=cos(2/nz*max(az,nz/2))*u
		if abs(j-bx)<2 then
			y=max(y,ry+u)
			if j==bx-1 then
				by=ry
				bw=3
				bc=13
			end
			if j==bx and a%2==0 then
				by=ry-uh
				bw=0.25
				bc=7
				bt=1
			end
			if y>ry and a%8==0 then
				y=ry+u
				c=6
			else
				y=max(y,ry)
			end
		end
		if j==bx-2 or j==bx+2 then
			if y>ry and a%16==0 then
				y=ry-u2
				c=6
				by=ry-u3
				bc=5
			end
			if y<ry then --wall
				c=a%4<2 and 6 or 13
			end
		end
		l[j]={
			x=(j-nx/2)*u,y=y,w=1,c=c,
			by=by,bw=bw,bh=bh,bc=bc,
			bt=bt,e=false}
	end
	--------------------------road
	rx=36
	if az==rx or az==rx+1 then
		for j=0,nx-1 do
			v=l[j]
			if abs(j-bx)>2
			and v.y<u then --tunnel
				v.by=v.y
				v.bh=-v.y+u
				v.bc=v.c
			end
			v.y=u3
			v.c=13
			if j>p-2 and j<p+3 then
				v.y=u4 --water
				v.c=1
			end
			if j==p-2 or j==p+3 then
				v.y=u3
				v.by=u2+uh
				v.bh=uh
				v.bc=5
			end
			if j==p-1 or j==p+2 then
				v.y=u3
				v.c=6
				v.by=u2
				v.bc=5
			end
			if j==p then
				v.by=u+uh
				v.bw=2
				v.bc=5
			end
		end
	end
	if az==rx-1 or az==rx+2 then
		for j=0,nx-1 do
			v=l[j]
			if v.y<u and abs(p-bx)>1 then
				v.by=v.y
				v.bh=-v.y+u
				v.bc=v.c
				v.y=u
				v.c=6
			end
		end
	end
	-----------------------objects
	obj(l,20,38,32) --tower
	obj(l,16,18,0)  --airport
	obj(l,2,10,16)  --pyramid
	vopt(l)         --optimize
	return l
end

function obj(l,p,n,o)
	local az,v,x,y,c
	az=tn%nz
	if az>n and az<n+9 then
		for j=0,7 do
			v=l[p+j]
			x=j+o+8
			y=n+8-az
			c=sget(j+o,y)
			if c!=0 then
				v.y=(4-sget(x,y))*u
				v.c=c
			end
			y+=8
			c=sget(j+o,y)
			if c!=0 then
				v.by=(4-sget(x,y))*u
				v.bc=c
			end
		end
	end
end

function vopt(l)
	local p,y,c=1,false,false
	for j=0,nx-1 do
		local v=l[j]
		if v.y==y and v.c==c then
			p+=1
		else
			p,y,c=voptl(l,p,j),v.y,v.c
		end
	end
	voptl(l,p,nx)
end

function voptl(l,p,j)
	if p>1 then
		l[j-p].w=p
		for k=j-p+1,j-1 do
			l[k].w,l[k].c=1,1
		end
		return 1
	end
	return p
end

function aam()
	local x,v
	x=mn%2==0 and suh or -suh
	v={
		x=-sx+x,y=-sy,z=sz,
		vx=x/2,vy=suh/2,vz=0.125,
		t={
			x=mx,y=my,z=d,
			w=0,h=0,l=false}
		}
	if(mt) v.t=mt
	add(msl,v)
	mn+=1
	sfx(2)
end

function fire()
	local x,y,z,v
	x=bn%2==0 and su2 or -su2
	y=bn%2==0 and ix/4 or -ix/4
	z=sz+4*su+rnd(0.75)*uz
	v={x=-sx+x,y=-sy-y,z=z}
	add(bul,v)
	smoke(v.x,v.y-su,z,0,9)
	bn+=1
	bf=2
	sfx(1)
end

function spawn(i,j)
	local y,w,h,e
	y=-u4-rnd(16)*u
	w=2+flr(rnd(10))
	h=2+flr(rnd(10))
	e={
		e=(w+h)*5,    --energy
		f=(w+h)*5,    --force
		k=false,
		l=false,      --lock
		t=t,          --time
		x=(-nx/2+j)*u+uh,
		y=y,
		z=tz[i],
		w=w*su,
		h=h*su,
		py=y,
		vy=su/4,
		ry=1.03125,
		lf=0,         --laser flash
		bl=8+rnd(16), --bolt length
		bt=rnd(232),  --bolt time
		a=rnd(360),
		v=tv[i][j]}
	add(foe,e)
	tv[i][j].e=e
end

function enemy(e)
	local zf,zu,x,y,w,h,g,c
	zf=1/e.z*f
	zu=zf*su2
	x,y=(px+e.x)*zf,(py+e.y)*zf
	w,h=e.w*zf-zu-1,e.h*zf-zu-1
	g=(py+e.v.y)*zf
	c={0,1,2,4,8,9,10,7}
	color(0)
	rectfill(x+w,y+zu,x-w,y-zu)
	rectfill(x+zu,y+h,x-zu,y-h)
	circfill(x,y,uh*zf)
	circfill(x-w,y,zu)
	circfill(x+w,y,zu)
	circfill(x,y-h,zu)
	circfill(x,y+h,zu)
	if e.lf>0 then
		color(c[flr(e.lf)])
		circfill(x,y,zu)
		e.lf-=0.25
	end
	if e.e>0 then
		color(12)
		if fc%32<16 and e.w>5*su
		and zu>1 then
			circfill(x-w,y,zu/2)
			circfill(x+w,y,zu/2)
		end
		if fc%32>15 and e.h>5*su
		and zu>1 then
			circfill(x,y-h,zu/2)
			circfill(x,y+h,zu/2)
		end
	end
	if e.e>0 and e.y>-16*u ---bolt
	and t%256>=e.bt
	and t%256<e.bt+e.bl then
		local n,x1,y1,x2,y2
		n,x1,y1=16,x,y+h+zu*2
		if fc%2==0 then
			color(12)
			line(x,y+h+zu,x,g)
		end
		color(7)
		circfill(x1,y1,zu/2)
		for i=0,n-2 do
			x2=x1+rnd(4*zu)-2*zu
			y2=y1+((e.v.y-e.y)/n*zf)
			line(x1,y1,x2,y2)
			x1,y1=x2,y2
		end
		if t%256>e.bt+e.bl-1 then
			e.bl=4+rnd(28)
			e.bt=rnd(256-e.bl)
		end
	end
end

function ec(e,x,y)
	if(not e) return false
	local w,h=abs(e.x-x),abs(e.y-y)
	return (w<e.w and h<su2)
	or (h<e.h and w<su2)
	or (w<uh and h<uh)
	and true or false
end

function laser(x,y,z,a,b)
	add(lsr,{
		x=x,y=y,z=z,
		a=a,b=b,r=0})
	sfx(4)
end

function boom(x,y,z,w)
	add(exp,{
		x=x,y=y,z=z,w=w,
		vx=(rnd(16)-8)/32,
		vy=(rnd(16)-8)/32,
		vz=(rnd(16)-8)/32})
end

function ex(x,y,z)
	for i=0,15 do
		boom(x,y,z,(8+rnd(4))/16*u)
	end
	sfx(3,-1,8/d*z)
	cs=2/d*(d-sz-z)
end

function smoke(x,y,z,w,c)
	add(smo,{
		i=0,
		x=x,y=y,z=z,w=w,c=c,
		vx=(rnd(12)-6)/256,
		vy=(rnd(12)-6)/256,
		vz=0})
end

function tc(i,j,y,z)
	local v=tv[i][j]
	if v and z>tz[i] then
		if(y>v.y) return 1
		if(y>v.by and y<v.by+v.bh) return 2
	end
	return 0
end

function dead()
	se,s,tw=0,0.5,64
	ex(-sx,-sy,sz)
	if(mt) mt.e=0
	for i=0,7 do
		if sc>scr[i] then
			for j=i,6 do
				scr[i+1]=scr[i]
			end
			scr[i]=sc
			break
		end
	end
	for i=0,7 do
		dset(i,scr[i])
	end
end

function game()
	local cx,cy,c,n,a,b,o
	local x,y,z,zf,zu,w,h,r,x2,y2
	px,py=sx*0.9375,sy*0.9375+u2
	a=t/10
	cx=-64+sx/4--+cs*cos(a)
	cy=-48+sy/4+cs*sin(a)
	ch=cy+120
	camera(cx,cy)
	clip(0,0,128,121)
	y=u2/d*f
	w=cx+127
	---------------------------sky
	c={15,14,13,5,1}
	y2=y
	h=2
	for i=0,5 do
		color(c[i])
		rectfill(w,y2,cx,y2-h-1)
		line(w,y2+2,cx,y2+2)
		y2-=h
		h*=1.5
	end
	rectfill(w,cy,cx,y2)
	-----------------------horizon
	color(7)
	line(w,y-1,cx,y-1)
	color(12)
	line(w,y,cx,y)
	---------------------------sea
	color(13)
	line(w,y+1,cx,y+1)
	color(1)
	rectfill(w,ch,cx,y+2)
	-------------------------stars
	c={7,6,13,13}
	for i=0,sn-1 do
		v=sta[i]
		x=(v.x-cx-sx/4)%128
		if(i%16) color(c[flr(i/8)+1])
		pset(cx+x,v.y-56+y)
	end
	-----------------------planets
	x=-sx/8-8
	spr(14,x,y-24,2,2) --moon
	x=-sx/6-8
	spr(42,x-32,y-17,4,2) --sun
	spr(47,x+16,y-32)  --mercury
	spr(46,x+48,y-24)  --venus
	x=-sx/4-8
	spr(62,x-76,y-19)  --titan
	spr(63,x-40,y-40)  --neptune
	-------------------------voxel
	for i=pz-1,0,-1 do dt(i) end
	for i=nz-1,pz,-1 do dt(i) end
	---------------------explosion
	c={7,10,9,8,4,5,13,6}
	for v in all(exp) do
		zf=1/v.z*f
		x,y=(px+v.x)*zf,(py+v.y)*zf
		w=v.w*zf
		color(c[min(flr(12/v.w),7)])
		circfill(x,y,w)
	end
	------------------------bullet
	color(9)
	for b in all(bul) do
		for i=0,3 do
			zf=1/(b.z+(2-i)*su)*f
			x,y=(px+b.x)*zf,(py+b.y)*zf
			w=su/8*zf
			rect(x+w,y+w,x-w,y-w)
		end
	end
	-----------------------missile
	c={8,8,8,5,9}
	for m in all(msl) do
		for i=0,3 do
			zf=1/(m.z+(2-i)*su)*f
			x,y=(px+m.x)*zf,(py+m.y)*zf
			w=su/4*zf
			color(c[i+1])
			rectfill(x+w,y+w,x-w,y-w)
		end
		if m.vz>suh then
			zf=1/(m.z-su2)*f
			x,y=(px+m.x)*zf,(py+m.y)*zf
			color(10)
			circfill(x,y,su*zf)
		end
	end
	------------------------smoke1
	for v in all(smo) do
		if v.z>sz then
			ds(v.x,v.y,v.z,v.w,v.c)
		end
	end
	-------------------------laser
	color(8)
	for l in all(lsr) do
		for i=0,3 do
			r=l.r+i*2
			z=l.z-r*cos(l.a)
			zf=1/z*f
			x=(px+l.x+r*sin(l.a))*zf
			y=(py+l.y+r*sin(l.b))*zf
			circfill(x,y,su*zf)
		end
	end
	---------------------spaceship
	if se>0 then
		for i=0,count(shp) do
			v=shp[i]
			zf=1/(sz-v.z)*f
			zu=zf*suh
			x=(v.x+px-sx+v.z*ix/4)*zf
			y=(v.y+py-sy-v.x*ix/4+v.z*iy/4)*zf
			if (v.c!=6 and v.c!=8 and v.c!=9) or i==1 then
				w,h=v.w*zf-1,v.h*zf-1
				color(v.c)
				rectfill(x+w,y+h,x,y)
			else
				if v.c==6 then
					color(6)
					circfill(x+zu,y+zu,zu)
				end
				if v.c==9 then
					w=zu-4
					spr(s<0.5 and 35 or 34,x+w,y+w)
				end
				if v.c==8 then
					w=zu-4
					if bf>0 then
						spr(36+flr(rnd(4)),x+w,y+w,1,1)
						bf-=1
					end
				end
			end
		end
		if sh>0 then ----------shield
			zf=1/sz*f
			x,y=(px-sx)*zf,(py-sy)*zf
			w=sh*su*zf
			color(12)
			circfill(x,y,w)
			color(6)
			circfill(x,y,w/3*2)
			color(7)
			circfill(x,y,w/3)
		end
	end
	------------------------smoke2
	for v in all(smo) do
		if v.z<=sz then
			ds(v.x,v.y,v.z,v.w,v.c)
		end
	end
	--------------------------mire
	c=8
	color(c)
	if my<-u4 then -----air to air
		if mt and mt.e>0 then
			zf=1/mt.z*f
			x=(px+mt.x)*zf
			y=(py+mt.y)*zf
			w=max(suh,mt.w)*zf
			h=max(suh,mt.h)*zf
			if(mt.l) c=11
			pal(8,c) --lock
			spr(12,x-w-1,y-h-1)
			spr(13,x+w-6,y-h-1)
			spr(28,x-w-1,y+h-6)
			spr(29,x+w-6,y+h-6)
			pal()
			color(c)
			print(flr(mt.z-sz),x+w+3,y-2)
			print(-flr(-mt.e/50),x-w-5,y-2)
			v=(w*2+4)/mt.f*mt.e
			color(0)
			rect(x+w+2,y+h+4,x-w-2,y+h+3)
			color(mt.e>mt.f/2 and 12 or 8)
			rect(x-w-2+v,y+h+4,x-w-2,y+h+3)
			if mt.l and ms!=1 then
				sfx(0)
				ms=1
			end
		else
			ms=0
		end
	else -----------air to surface
		zf=1/mz*f
		zu=zf*u
		x=(px+mx-mx%u)*zf
		y=(py+my-my%uh)*zf
		rect(x+zu,y+zu,x-1,y-1)
		if mz<d then
			print(flr(mz-sz),x+zu+2,y+zu/2-2)
		end
	end
	for m in all(msl) do
		v=m.t
		if v.l and v.y>-u4 then
			zf=(1/v.z)*f
			zu=zf*u
			x=(px+v.x-v.x%u)*zf
			y=(py+v.y-v.y%uh)*zf
			color(11)
			rect(x+zu,y+zu,x-1,y-1)
		end
	end
	zf=1/mz*f
	x,y=(px+mx)*zf,(py+my)*zf
	r=1
	c=ec(mt,mx,my) and 11 or 8
	color(c)
	if mt and mz!=d and se>0 then
		r=(mt.w+mt.h)*zf
		zf=1/(sz+u)*f
		x2,y2=(px-sx)*zf,(py-sy)*zf
		n=(x2-x)/(y2-y)
		for i=0,y2-y-1,2 do
			pset(x+n*i,y+i)
		end
	end
	-----------------------pointer
	for i=0,r/2,2 do
		pset(x-r+i,y)
		pset(x+r-i,y)
		pset(x,y-r+i)
		pset(x,y+r-i)
	end
	if(btn(5)) circ(x,y,r+2)
	---------------------------hud
	camera(0,0)
	color(0)
	rectfill(8,24,4,1)
	color(7)
	for i=0,3 do
		y=1+(8*i+py+4)%24
		pset(5,y)
		y=1+(8*i+py)%24
		line(5,y,6,y)
	end
	y=13
	color(8)
	line(5,13,6,13)
	color(0)
	print(flr(sy+u4),10,11)
	--print(flr(-sx),10,5)
	-------------------------speed
	y=12*s-1
	rectfill(1,1,2,24)
	if y>0 then
		color(2)
		rectfill(1,24-y,2,24)
	end
	--color(8)
	--pset(1,24-y)
	--pset(2,24-y)
	--if dbg then
	--	color(0)
	--	print("lvl="..(lvl+1),103,21)
	--	print("er="..er,103,27)
	--	print("lr="..lr,103,33)
	--end
	---------------------------low
	if (mt and mt.z-sz<4*uz)
	and t%16<8 then
		spr(124,48,0,4,1)
	end
	--------------------------dead
	if se<1 then
		c={0,1,2,5,8,8,8,8}
		camera(-46,-52)
		pal(8,c[4+flr(4*sin(t/64))])
		spr(77)
		spr(78,9)
		spr(79,18)
		spr(77,27)
		pal(1,0)
		spr(92,0,9,2,1)
		spr(94,20,9,2,1)
		pal()
		if tw==0 and t%64<32 then
			color(0)
			print("pressŽ—",-45,63)
		end
	end
	---------------------------bar
	camera(0,-121)
	clip()
	color(0)
	rectfill(127,6,0,0)
	spr(48,56,0,2,1) --eagle
	spr(se>0 and 50 or 51,-1)
	o=0
	if(se==0 or (se<25 and t%64<32)) o=-16
	nbr(se,7,1,3,100,o)
	spr(54,88)
	nbr(sc,96,1,4,1000,0)
	-------------------------debug
	if(not dbg) return
	color(5)
	spr(52,28)
	print(count(msl),35,1)
	spr(53,36)
	print(count(lsr),43,1)
	spr(65,72)
	print(count(smo),78,1)
end

function dt(i)
	if(tz[i]<4) return
	local zi,zf,zu,x,y,w,h,e,v
	zi=tz[i]
	zf=1/zi*f
	zu=zf*u
	e=false
	if zi>d-3*uz then
		x=flr(((d-zi)/uz)*2)
		for j=0,15 do
			pal(j,sget(97+x,80+j))
		end
	end
	for j=0,nx-1 do
		v=tv[i][j]
		if v.c!=1 then
			y=(v.y+py)*zf
			if y<ch then
				x=(v.x+px)*zf
				w=v.w*zu
				h=(u4-v.y)*zf
				color(v.c)
				rectfill(x+w-1,min(y+h-1,ch),x,y)
				--color(8)
				--pset(x+zu/2-1,y-1)
			end
		end
		if v.by<u4 then
			y=(v.by+py)*zf
			if y<ch then
				color(v.bc)
				if v.bt==0 then
					x=(v.x+px)*zf
					w=v.bw*zu-1
					h=v.bh*zf-1
					rectfill(x+w,min(y+h,ch),x,y)
				else
					x=(v.x+px+uh)*zf
					w=(v.bw/2)*zu
					circfill(x,y+uh*zf,w)
				end
			end
		end
		if(v.e) e=j
	end
	if(e) enemy(tv[i][e].e)
	if(zi>d-3*uz) pal()
end

function ds(x,y,z,w,c)
	local zf,i=1/z*f,64+flr(rnd(2))*16
	x,y=(px+x)*zf-3,(py+y)*zf-3
	pal(6,c)
	spr(i+min(w*su*zf,7),x,y)
	pal()
end

function upd()
	local x,y,z,w,v,a,b,c
	if(sh>0) sh-=1
	if(cs>0) cs-=0.0625
	if(abs(cs)<0.0625) cs=0
	s=0.0078125*(32+sy+u4) --speed
	if(dbg and btn(0,1)) s=0
	-------------------------level
	lvl=flr(t/1024)
	er=max(16-lvl,4)
	lr=max(128-lvl*4,32)
	-----------------------terrain
	for i=0,nz-1 do
		tz[i]-=s
		if tz[i]<s then
			tz[i]+=d
			pz+=1
			if(pz>nz-1) pz=0
			tv[i]=voxel(tn)
			if pz%er==0 then
				spawn(i,flr(2+rnd(nx-4)))
			end
			tn+=1
		end
	end
	-------------------------smoke
	if se>0 then
		y=-sy-suh
		z=sz-su2-rnd(0.75)*uz
		if s>0.25 then
			if fc%2==0 then
				smoke(-sx-su,y+ix/4,z,0.125,6)
			else
				smoke(-sx+su,y-ix/4,z,0.125,6)
			end
		end
		if se<=50 and
		fc%flr(se/5)==0 then
			x=-sx+(3-rnd(6))*su
			smoke(x,-sy,sz,0.25,0)
		end
	end
	if fc%2==0 then
		for m in all(msl) do
			if m.vz>suh then
				smoke(m.x,m.y,m.z-su2,su,7)
			end
		end
	end
	for v in all(smo) do
		v.i+=1
		v.x+=v.vx
		v.y+=v.vy
		v.z-=s
		if v.i<16 then
			v.w+=0.03125 --1/32
		else
			v.w-=0.015625 --1/64
		end
		if(v.z<2 or v.w<0) del(smo,v)
	end
	-------------------------enemy
	for e in all(foe) do
		e.z-=s
		if(e.z<4) del(foe,e)
		if e.e<1 then
			if(mt==e) mt=false
			if fc%2==0 then
				x=e.x+rnd(e.w*2)-e.w
				smoke(x,e.y,e.z,su,0)
			end
			e.y+=e.vy
			e.vy*=e.ry
			if e.y>e.v.y then --shutdown
				e.k=true
				v=e.v
				v.by,v.bc,v.bt=v.y,0,0
				v.e=false --no reference?
				del(foe,e)
				ex(e.x,v.y,e.z)
			end
		end
		if se>0 and s>0
		and e.z>sz+su2
		and flr(t-e.t)%lr==0 then
			z=e.z-sz-4*uz*s
			b=0.25+atan2(e.x+sx,z)
			c=0.25+atan2(e.y+sy,z)
			laser(e.x,e.y,e.z,b,c)
			e.lf=7
		end
	end
	-------------------------laser
	for l in all(lsr) do
		l.z-=s
		l.r+=4
		x=l.x+l.r*sin(l.a)
		y=l.y+l.r*sin(l.b)
		z=l.z-l.r*cos(l.a)
		if se>0 and z<sz+su2
		and abs(-sx-x)<su2
		and abs(-sy-y)<su2 then
			del(lsr,l)
			se-=5
			s,sh,cs=0.125,8,1.5
			sfx(5)
			if(se<1) dead()
		end
		if(abs(x)>nx/2*u or z<4) del(lsr,l)
	end
	---------------------spaceship
	for e in all(foe) do
		if se>0 and sz>e.z
		and ec(e,-sx,-sy) then
			e.e=0
			dead()
		end
	end
	--------------------------mire
	mx,my,mz=-sx,-sy,d
	if my<-u4 then -----air to air
		for e in all(foe) do
			if abs(e.x-mx)<e.w+8 and
			abs(e.y-my)<e.h+8 then
				mz=e.z
				mt=e
				if abs(e.x-mx)<e.w and
				abs(e.y-my)<e.h then
					mt.l=true
				end
				break
			end
		end
	else -----------air to surface
		j=flr((-sx+nx/2*u)/u)
		for i=pz,nz-1 do
			v=tv[i][j]
			if -sy>v.y or (-sy>v.by
			and -sy<v.by+v.bh) then
				mz=tz[i]
				goto me
			end
		end
		for i=0,pz-1 do
			v=tv[i][j]
			if -sy>v.y or (-sy>v.by
			and -sy<v.by+v.bh) then
				mz=tz[i]
				goto me
			end
		end
		::me::
		if mz<d then
			mt={
				x=mx,y=my,z=mz,w=uh,h=uh,
				e=0,l=true}
		end
		if(se>0 and sz+su2>mz) dead()
	end
	if(mt and mt.z<sz+su2) mt=false
	------------------------bullet
	for b in all(bul) do
		b.z+=uz-s
		if b.y<-u4 then --air to air
			for e in all(foe) do
				if b.z>e.z
				and ec(e,b.x,b.y) then
					del(bul,b)
					boom(b.x,b.y,e.z,uh)
					if e.e>0 then
						e.e-=10
						if e.e<1 then
							ex(e.x,e.y,e.z)
							sc+=flr(e.f/5)
						else
							sc+=1
						end
					end
				end
			end
		else ----------air to surface
			local j,ctv,cti,v2=0,0,0
			j=flr((b.x+nx/2*u)/u)
			for i=pz,nz-1 do
				ctv=tc(i,j,b.y,b.z)
				if ctv!=0 then
					cti=i
					goto be
				end
			end
			for i=0,pz-1 do
				ctv=tc(i,j,b.y,b.z)
				if ctv!=0 then
					cti=i
					goto be
				end
			end
			::be::
			if ctv!=0 then
				del(bul,b)
				smoke(b.x,b.y,b.z,su2,5)
			end
		end
		if(b.z>d) del(bul,b)
	end
	-----------------------missile
	for m in all(msl) do
		local v,x,y=m.t,0,0
		if v.l then
			z=m.vz/32
			x=(v.x-m.x)*z
			y=(v.y-m.y)*z
		end
		m.x+=m.vx+x
		m.y+=m.vy+y
		m.z+=m.vz
		m.vz*=1.05
		if(v.y>-u4) v.z-=s
		if(m.z>d) del(msl,m)
		if not v.l and m.z>d/2 then
			del(msl,m)
			ex(m.x,m.y,m.z)
		end
		if m.y<-u4 then --air to air
			if abs(v.z-m.z)<m.vz
			and ec(v,m.x,m.y) then
				del(msl,m)
				ex(v.x,v.y,v.z)
				if v.e>0 then
					v.e-=50
					if(v.e<1) sc+=flr(v.f/5)
				end
			end
		else ----------air to surface
			local j,ctv,cti,v2=0,0,0
			j=flr((m.x+nx/2*u)/u)
			for i=pz,nz-1 do
				ctv=tc(i,j,m.y,m.z)
				if ctv!=0 then
					cti=i
					goto me
				end
			end
			for i=0,pz-1 do
				ctv=tc(i,j,m.y,m.z)
				if ctv!=0 then
					cti=i
					goto me
				end
			end
			::me::
			if ctv!=0 then
				del(msl,m)
				ex(m.x,m.y,m.z)
				v=tv[cti][j]
				if v.w>1 then
					v2=tv[cti][j+1]
					v2.w=v.w-1
					v2.y=v.y
					v2.c=v.c
				end
				if ctv==1 then
					v.y=m.y+u
					v.by=m.y
					v.bh=u
					v.bc=0
					v.bt=0
				else
					v.by=u4
				end
			end
		end
	end
	---------------------explosion
	for v in all(exp) do
		v.x+=v.vx
		v.y+=v.vy
		v.z+=v.vz-s
		v.w-=u/64
		if v.z<4 or v.w<0 then
			del(exp,v)
		end
	end
end

function menu()
	local cx,cy,n,c,c1,c2,y,o
	cx,cy=-64,-64
	camera(cx,cy)
	color(0)
	rectfill(cx+127,cy+127,cx,cy)
	a=t/1080
	sf()
	if mode==1 then ----------logo
		spr(72,25,57,5,1)
		camera(-16,-55)
		n=flr(t/4)%128
		for j=0,15 do
			c1=sget(n+j,64)
			c2=sget(n-j,65)
			for i=0,92 do
				c=sget(i,80+j)
				if c==3 and c1!=0 then
					pset(i,j,c1)
				end
				if c==11 and c2!=0 then
					pset(i,j,c2)
				end
			end
		end
		camera(-30,-71)
		for j=0,4 do
			c1=sget(n+j,66)
			for i=0,67 do
				c=sget(i,72+j)
				if c==11 and c1!=0 then
					pset(i,j,c1)
				end
			end
		end
	else --------------------score
		for i=0,7 do tri(i) end
		camera(-44,-34)
		spr(106,0,0,5,1)
		for i=0,7 do
			y=12+i*6
			spr(122,-8,y)
			nbr(i+1,0,y,1,1,0)
			spr(123,8,y)
			o=0
			if(sc!=0 and sc==scr[i] and t%64<32) o=-16
			nbr(scr[i],16,y,4,1000,o)
		end
	end
	camera(0)
	color(1)
	print("pressŽ—",1,122)
end

function sf() --------starfield
	local v,x,y,z,b,zf,c,k,n
	c={1,2,14,15,7,8,
				1,5,13,12,7,8}
	for i=0,sn-1 do
		v=sta[i]
		b=v.a+cos(a)/4+(sx+sy)/720
		x=v.r*cos(b)+128*cos(a)-sx
		y=v.r*sin(b)+128*cos(a/2)-sy
		z=(v.z+1024*sin(a/3))%256
		zf=1/z*32
		x,y=x*zf,y*zf
		v.t[0]={x=x,y=y,z=z}
		for j=5,1,-1 do
			v.t[j]=v.t[j-1]
		end
		if abs(v.t[0].x-v.t[5].x)<64
		and abs(v.t[0].y-v.t[5].y)<64
		and abs(v.t[0].z-v.t[5].z)<200
		and abs(x)<96 and abs(y)<96 then
			k=6-flr(z/48)
			n=i%2*6+k
			if k>2 then
				for j=k-1,1,-1 do
					color(c[n-j])
					line(v.t[j].x,v.t[j].y,v.t[j-1].x,v.t[j-1].y)
				end
			else
				color(c[n])
				pset(x,y)
			end
		end
	end
end

function tri(i)
	local v,b,r,x,y,z,x2,y2,zf
	local c={1,2,4,8,8,8,8,8}
	x2,y2={},{}
	for j=0,11 do
		r=(j<6) and 20 or 28
		v=sta[sn+i*12+j]
		b=v.a+cos(a)/4+(sx+sy)/720
		v.x,v.y=r*cos(b),r*sin(b)
		x=v.x+128*cos(a)-sx
		y=v.y+128*cos(a/2)-sy
		z=(256-i*32+1024*sin(a/3))%256
		zf=1/z*32
		x2[j],y2[j]=x*zf,y*zf
	end
	color(c[8-flr(z/32)])
	--line(x2[0],y2[0],x2[5],y2[5])
	line(x2[6],y2[6],x2[11],y2[11])
	for j=1,5 do
		if(j%2==1) line(x2[j-1],y2[j-1],x2[j],y2[j])
		line(x2[j+5],y2[j+5],x2[j+6],y2[j+6])
	end
end

function nbr(n,x,y,l,k,o)
	for i=0,l-1 do
		spr(112+o+((n/k)%10),x+i*8,y)
		k/=10
	end
end

function db()
	dbg=not dbg
	dset(8,dbg and 1 or 0)
end

function _draw()
	if mode==0 then
		game()
	else
		menu()
	end
	fc+=1
	if dbg then
		local x,y,n
		camera(0,0)
		--------------------------fps
		n=100-flr(100/stat(1))
		fps[fc%24]=max(0,n)
		color(0)
		rectfill(126,1,103,16)
		color(1)
		print(fps[fc%24],104,2)
		color(2)
		line(103,8,126,8)
		for i=0,23 do
			v=fps[(i+fc%24+1)%24]
			if v>0 then
				color(1)
				x=103+i
				y=17-v*0.16
				line(x,y,x,16)
				color(v>50 and 8 or 12)
				pset(x,y)
			end
		end
		--------------------------mem
		n=stat(0)*0.0234375 --24/1024
		color(0)
		rectfill(126,18,103,19)
		color(2)
		rectfill(102+n,18,103,19)
	end
end

function _update60()
	t=time()*32-tb
	if(mode==0) upd()
	if(tw>0) tw-=1
	-------------------------input
	local xm,yt,im,ii,id,iz
	xm=18*u      --x max
	yt=24*u      --y top
	im=2
	ii=0.125
	id=0.0625    --1/16
	if se>0 and s<1 then
		s+=0.015625 --1/64
	end
	if(btn(0) and ix<im) ix+=ii
	if(btn(1) and ix>-im) ix-=ii
	if(btn(2) and iy<im) iy+=ii
	if(btn(3) and iy>-im) iy-=ii
	if mode==0 and se>0 then
		if btn(4) then --fire missile
			if mp%32==0 and   --rate
			count(msl)<4 then --max
				aam()
			end
			mp+=1
		else
			mp=0
		end
		if btn(5) then ---fire bullet
			if(bp%3==0) fire()
			bp+=1
		else
			bp=0
		end
	end
	if (btnp(4) or btnp(5))
	and tw==0 then
		if mode==0 and se<1 then
			init(2)
			music(0)
			return
		end
		if mode==1 then
			init(0)
			music(-1)
			return
		end
		if mode==2 then
			mode=1
			return
		end
	end
	if(btnp(4,1)) db() ------debug
	----------------------position
	if(ix>0) ix-=id
	if ix>im/2 and sx>xm-u3 then
		ix-=2*id
	end
	if(ix<0) ix+=id
	if ix<-im/2 and sx<-xm+u3 then
		ix+=2*id
	end
	if(iy>0) iy-=id
	if iy>im/2 and sy>yt-u3 then
		iy-=2*id
	end
	if(iy<0) iy+=id
	if iy<-im/2 and sy<u then
		iy+=2*id
	end
	if(abs(ix)<id) ix=0
	if(abs(iy)<id) iy=0
	if abs(sx+ix)<xm then
		sx+=ix
	else
	 ix=0
	end
	if sy+iy>-u2 and sy+iy<yt then
		sy+=iy
	else
		iy=0
	end
end
__gfx__
55500000333000002222222211111111000000000000000000000000000000000000000000000000000000000000000008800000000008800000001212200000
5d5ddd00373222002eeeeee212222221000dd0000001100000000000000000000000000000000000000000000000000080000000000000080000d11111128000
555555d0333222202e2222e212333321005555000022220000000000000000000000000000000000000000000000000080000000000000080006511111114900
0d55755d022222222e2ff2e2123443210d5dd5d0012772100000000000000000000000000000000000000000000000000000000000000000006d111111111480
0d57775d022222222e2ff2e2123443210d5dd5d00127721000000000000000000000000000000000000000000000000000000000000000000d65111111111120
0d55755d022222222e2222e212333321005555000022220000000000000000000000000000000000000000000000000000000000000000000661111111111112
00d555d0002222202eeeeee212222221000dd0000001100000000000000000000000000000000000000000000000000000000000000000005661111111111112
000ddd0000022200222222221111111100000000000000000000000000000000000000000000000000000000000000000000000000000000d665111111111111
0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000677d555555555555
0b000000060000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000005666111111111111
0000000000000000000000000000000000dddd00007777000000000000000000000000000000000000000000000000000000000000000000077765555555555d
0000000000000000000000000000000000dccd000078870000000000000000000000000000000000000000000000000000000000000000000d77765555555560
0000000000000000000000000000000000dccd000078870000000000000000000000000000000000000000000000000000000000000000000067776d555d6600
0000000000000000000000000000000000dddd000077770000000000000000000000000000000000000000000000000080000000000000080006777777776000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000080000000000000080000d677776d0000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000008800000000008800000000000000000
00000000000000000000000000000000000090000000090090000009009000000000000000000000000000000000000000000000000000000000000000000000
080208000000000000555500005555000009a00000009a000a9009a000a9000000000000000000000000000000000000000000000000000000dccd0000012100
06020600000c000005499450055115500009a0009a99a90009a99a90009a99a90000000000000000000000000000000000000000000000000dccccd000149410
06d2d60000020000059aa950051111509aaaa99009aaa900009aa900009aaa900000000000000000000000000000000000000000000000000cccccc00029a920
02d2d20000000000059aa95005111150099aaaa9009aaa90009aa90009aaa9000000000000000000000000000000000000000000000000000cccccc000149410
22525220e00e00e00549945005511550000a9000009a99a909a99a909a99a9000000000000000000000000000000000000000000000000000dccccd000012100
20909020e00e00e00055550000555500000a900000a900000a9009a000009a000000000000000000000000000000eeeeeeee00000000000000dccd0000000000
00000000000000000000000000000000000900000090000090000009000009000000000000000000000000000eeeeeeeeeeeeee0000000000000000000000000
000000000000000000000000000000000000000000000000000000000000000000000000000000000000000ffffffffffffffffff00000000000000012100000
49aaa940049aaa94002818200057765000018100000000000097a940000000000000000000000000000000eeeeeeeeeeeeeeeeeeee0000000055550028200000
124a92000029a421008e888000766d6000028200000282000047a92000000000000000000000000000000ffffffffffffffffffffff00000053bb35012100000
04a9400484049a40002888200070d060000282000008a800001494100000000000000000000000000000ffffffffffffffffffffffff00000dbbbbd000000000
002a924a9129a2000002820000570650000888000002820000029200000000000000000000000000000ffffffffffffffffffffffffff00005bbbb5000000000
000149a42494100000002000000676000002a2000000000000499920000000000000000000000000000777777777777777777777777770000d3bb3d0000002d2
0000000000000000000000000000000000000000000000000000000000000000000000000000000000ffffffffffffffffffffffffffff0000dddd0000000d6d
000000000000000000000000000000000000000000000000000000000000000000000000000000000077777777777777777777777777770000000000000002d2
0000000000000000000000000000000000000000000600000006060006060600c50dcd100000d000000000d00000000000000000888880000008888800088880
0000000000000000000000000006000000006000006060000060606060606060ccccccc056606650806650666060606650566000888888000088888800888888
0000000000060000000060000060600000060600060606000606060006060600dccccc106000606060606060006060606066d000880088800888000008880088
00060000006060000006060006060600006060606060606060606060606060601dcccc0060006060606060600060606060600000880008888880088888800088
000000000006000000606000006060000606060006060600060606000606060005ccc5005dd0d0d0d0dd505dd05d50d0d05dd000880000888800888888888888
0000000000000000000600000006000000606000006060006060600060606060cccd50000000000000d000000000000000000000880000888800000088888888
00000000000000000000000000000000000600000006000006060000060606000000000000000000000000000000000000000000888888888888888888000088
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000888888800888888888000088
00000000000000000000000000000000000000000006000006060000060606000000000000000000000000000000000011101110111011101110101011101110
00000000000000000000000000060000000600000060600060606000606060600000000000000000000000000000000010001010101000001010101000000010
00000000000600000006000000606000006060000606060006060600060606000000000000000000000000000000000010101110101011101010101011101100
00060000006060000060600006060600060606006060606060606060606060600000000000000000000000000000000010101010101010001010101010001010
00000000000600000006060000606000006060600606060006060600060606000000000000000000000000000000000011101010101011101110011011101010
00000000000000000000600000060000000606000060600000606060606060600000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000060000006000000060600060606000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
288888208888200088888820888888208800088088888880288888808888888028888820288888205ddddd505ddddd505ddddd505ddddd505ddddd505ddddd50
88000880002880000000088000000880882008808800000088000000000008808800088088000880ee200000ee000000ee000ee0ee000ee0ee00000000002e20
990009900009900029999920000999202999999099999920999999200029992029999920299999902efffe20ff200000ff200ff0ff20ef20ff20e00002fff200
8800088000088000880000000000088000000880000008808800088000882000880008800000088000002ee0eee00000eee00ee0eee02ee0eee000002ee20000
144444104444444044444440444444100000044044444410144444100044000014444410444444109aaaa94049aaaa9049aaa9409aa00a9049aaaa9049aaaa90
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
56666650666650006666665066666650660006606666666056666660666666605666665056666650000056606650000000000000000000000000000000000000
66000660005660000000066000000660665006606600000066000000000006606600066066000660000066000660000008888808800008888808888808888800
77000770000770005777775000077750577777707777775077777750005777505777775057777770000077000770000008800008800008808808800008800000
66000660000660006600000000000660000006600000066066000660006650006600066000000660000066000660000008800008800008808808888808888000
1ddddd10ddddddd0ddddddd0dddddd1000000dd0dddddd101ddddd1000dd00001ddddd10dddddd1000001dd0dd10000008800008800008808800008808800000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000008888808888808888808888808888800
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0000000000000000125def7a9845215d676d51000000000000000000113b7b3110000000000000000000000012489999999999a77a9999999999842100000000
0000000015d67d12489ae21000000000000000000000113b7b311000000000000000000000000000000000000000000000000000000000000000000000000000
0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000125dddddddddddddddddddddddddddddc6a98421
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0bbb000000bbbb00bbb00b000b00bbbb0000bbbb00b000b0000bbbb000bbbb0bbbbb000000000000000000000000000000000000000000000000000000000000
b000b0000b00000b000b0bb0bb0b00000000b000b00b0b00000b000b0b00000000b0000000000000000000000000000000000000000000000000000000000000
bbbbb0000b0bbb0bbbbb0b0b0b0bbb000000bbbb0000b000000bbbb00bbb00000b00000000000000000000000000000000000000000000000000000000000000
b000b0000b000b0b000b0b000b0b00000000b000b000b000000b000b0b000000b000000000000000000000000000000000000000000000000000000000000000
b000b00000bbbb0b000b0b000b00bbbb0000bbbb0000b000000b000b00bbbb0bbbbb000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000300000000000000000000000000000000000000000000000
33333333333333330033333333333333003333333333300333333333333333333003333333333000330000000000300011111110000000000000000000000000
03bbbbbbbbbbbb3003bbbbbbbbbbbb3003bbbbbbbbbbb3003bbbbbbbbbbbbbb3003bbbbbbbbbb3003b3000000003300022222220000000000000000000000000
003bbbbbbbbbb30003bbbbbbbbbbb30003bbbbbbbbbbbb3003bbbbbbbbbbbb3003bbbbbbbbbbbb303bb30000003b300055533330000000000000000000000000
000333333bbb300003bb33333333300003bb33333333bb300033333bb333330003bbb333333bbb303bbb300003bb300055544440000000000000000000000000
00000003bbb3000003bb33333300000003bb3333333bbb300000003bb300000003bb30000003bb303bbbb30003bb300055555550000000000000000000000000
0000003bbb30000003bbbbbb3000000003bbbbbbbbbbb3000000003bb300000003bb30000003bb303bb3bb3003bb3000555ddd60000000000000000000000000
000003bbb300000003bbbbb30000000003bbbbbbbbbb30000000003bb300000003bb30000003bb303bb33bb303bb300055dd6670000000000000000000000000
00003bbb3000000003bb33300000000003bb3333333300000000003bb300000003bb30000003bb303bb303bb33bb300055544480000000000000000000000000
0003bbb33333330003bb33333333300003bb3000000000000000003bb300000003bbb333333bbb303bb3003bb3bb300055544490000000000000000000000000
003bbbbbbbbbbb3003bbbbbbbbbbb30003bb3000000000000000003bb300000003bbbbbbbbbbbb303bb30003bbbb3000554499a0000000000000000000000000
03bbbbbbbbbbbbb3003bbbbbbbbbbb3003bb3000000000000000003bb3000000003bbbbbbbbbb3003bb300003bbb3000555333b0000000000000000000000000
33333333333333333003333333333333003b3000000000000000003b3000000000033333333330003b30000003bb3000555dddc0000000000000000000000000
0000000000000000000000000000000000033000000000000000003300000000000000000000000033000000003b3000555555d0000000000000000000000000
000000000000000000000000000000000000300000000000000000300000000000000000000000003000000000033000555ddde0000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000300055ddeef0000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
__label__
11111111111111111111111111111111111111111111111111111111211111111111111111111111111161111111111111111111111111111111111111111111
100100000111111111111111111111111111111111d1111111111111111d11111111111111111111111111111111111111111111111111111111111111111111
10010700011111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111
1001000001111111111111111111111111111111111111111111111111d12d211111111111111111111111111111111111111111111111111111111111111111
100100000111111111111111111111111111111111111111111111111111d6d11111111111111117111111111111111111111111111111111111111111111111
1001000001111111111111111111111111111111111111111111111111172d211111111111111111111111111111111111111111111111111111111111111111
10010770011111111111111d1111111111111111111111111111111111111dd11111111111111171111111111111111111111111111111111111111111111111
10010000011111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111121111111111111111111
10010000011111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111494111111111111111111
1001000001111111111111111111111111111111111d1111111111111111111111111111111111111111111111111111111111111129a9211111111111111111
10010700011111111111111111111111111111111111111111111111111111111111111111111117111111111111111111111111111494111111111111111111
10010000010001000111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111121111111111111111111
50050000050505050555555555555555555555555555555555555555555555555555555555555555755555555555555555555555555555555555555555555555
12210880010001000111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111
5225077005050505055555555555555555555555555555555555555555555555500005555575555555555555555121225555555555555555555555555555555d
52250000050005000555555555555555555555555555555555555555555555555098055555555555555555555d11111128555555555555555555555555555555
52250000055555555555555555555555555555555555555555555555555555555008055555555655555555556511111114955555555555555555555555555555
5225000005555555555555555555555555555555555555555555555555555555555555555555555555555556d111111111485555555555555555555555555555
52250700055555555555555555555555555555555555555555555555555555555555555555555555555555d65111111111125555555555555555555555555555
52250000055555555555555555555555555555555555555555555555555555555555555555555555555555661111111111112555555555555555555555555555
52250000075555555555555555555555555555555655555555555555555555555555555555555555555555661111111111112555555555555555555555555555
5225000005555555555553bb3555555555555555555555555555555555555555555555555555555555555d665111111111111555555555555555555555555555
d22d07700ddddddddddddbbbbdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd677d555555555555ddddddddddddddddddddddddddd
522500000555555555555bbbb5555555555555555555555555555555555555755555555555555555555555666111111111111555555555555555555555555555
d22d00000dddddddddddd3bb3ddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd77765555555555dddddddddddddddddddddddddddd
ddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd7776555555556dddddddddddddddddddddddddddd
ddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd67776d555d66ddd6ddddddddddddddddddddddddd
ddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddeeeeeeeedddddddddddd6777777776dddddddddddddddddddddddddddddd
dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddeeeeeeeeeeeeeeddddddddddd677776dddddddddddddddddddddddddddddddd
eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeffffffffffffffffffeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee
ddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddeeeeeeeeeeeeeeeeeeeedddddddddddddddddddddddddddddddddddddddddddddd
eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeffffffffffffffffffffffeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee
eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeffffffffffffffffffffffffeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee
eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeffffffffffffffffffffffffffeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee
fffffffffffffffffffffffffffffffffffffffffffffffffffffffffff77777777777777777777777777fbfffffffffffffffffffffffffffffffffffffffff
eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeffffffffffffffffffffffffffffeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee
ffffffffffffffffffffffffffffffffffffffffffffffffffffffffff7777777777777777777777777777bbfffffbbfffffffffffffffffffffffffffffffff
7777777777777777777777777777777777777777777777777777777777777677767777777777777777777b777777777b77777777777777777777777777777777
ccccccccccccccccccccccccccccccccccccccccccccccccccccccccc6c6666ccccccccccccccccccbbbcbccc000cccbcbbccbbbcbcbcccccccccccccccccccc
dddddddddddddddddddddddddddddddddddddddddddddddddddddddddd6666dddddddddddddddddddddbdddd00800dddddbddbdbdbdbdddddddddddddddddddd
111111111111111111111111111111111111111111111111111111111116111111111111111111111bbb11100888001111b11bbb1bbb11111111111111111111
1111111111111111111111111111111111111111111111111111111111116111111111111111111b1b1111b1008b0b1111b11b1b111b11111111111111111111
111111111111111111111111111111111111111111111111111111111116661111111111111111111bbb1b1110a0111b1bbb1bbb111b11111111111111111111
1111111111111111111111111111111111111111111111111111111161116111111111111111111111111b1b1118111b11111111111111111111111111111111
11111111111111111111111111111111111111111111111111111116661111111111111111111111111111bb11181bb111111111111111111111111111111111
1111111111111111111111111111111111111111111111111111111161155161111111111111111111111111b117111111111111111111111111111111111111
111111111111111111111111111111111111111111111111111111111115566611111111111111111111ccbcccccccccc1111111111111111111111111111111
111111111111111111111111111111111554411111111111111111111113316111111111111111111111cccccbccccccc1111111111111111111111111111111
11111111111111111111111111111111499444411111111111111177777777776111111111111111111111b11111711111111111111111111111111111111111
11111111111111111111111111111114499444411111111111117777777777766677771111111111111111111b17771111111111111111111111111111111111
11111111111111111111111111111144999994551111111155517777777777676177771111111111111111111117771111111111111111111111111111111111
111111111111111111111111111114999999945541111111555177777777777777777711111111111111111111b171111999aaa1111111111111111111111111
11111111111111111111111111111449944994554111111155577777777777777777771111111111111dd441111111111999aaa9911111111111111111111111
11111111111111111111111111114449944444664111111166677777777777777777771111111111111dd445511b714999999999991111111111111111111111
1111111111111111111111111111444444444466555111dd67777777777777777777777771111551111dd5555557477444999999991111111111111111111111
11111111111111111111111111155544445555665551dddd67777777777777777777777771111dd1111dd5555555b747449999985a9411111111111111111111
1111111111111111111fffffff15554444555566533ddddd67777777777777777777777771155dd1111dd5555555757444944499aaa411111111111111111111
1111111111111111111fffffff3555555555556655dddddd677777777777777777777aaa71133dd1111dd55533554b544aaaaaaa4a4441111111111111111111
11aaaaaaa1111111112fffffff2222553333336ddddddddd67aaaa77777777777aaa7aaa79533dd1111dd333333335aaaaaaaaaa444445111111111111111111
aaaaaaaaaaaa1111122ffffff2222255333333dddd7ddddaaaaaaaaaaaaa77777aaa7aaa79133dd11116633333337baaaaaaa777774475511111111111111111
aaaaaaaaaaaa999222222222222222eeee333ddddddddddaaccccccccccc77777aaa7aaa79111dd111166333333777aaaaaaa777774757511111111111111111
aaaaaaaaaaaa99999922222222222eeeee2dddd7dddddd5cccccccccccccaaaa7aaa7aaa7ddd111111166333333777baa7777777777477531111111111111111
aaaaaaaaa999999999222222222eeeeeddddd7ddddddd53cccccccccccccaaaa7aaa7aaa7dddd111111111113399777aa7777777777775731111111111111111
aaaa5555599999999922225555eeeeeddddddddddddd544cccccccccccccaaaa7aaa7aaa7dddd111111111111199979ba7777777777757531111111111111111
aaa95555599999944444ee5555eeedddd7ddddddd3333ddcccccccccccccddddddda99996666d111111111111199999aa7777777777777533111111111111111
99995555599994444444445555eddddddddddddd333ddddccccccccccccdddddddd999996666d1111111111444999999b7777777777777777111111111111111
99995555599994444444445555dddd7ddddddd33333ddddccccccccccccdddddddd999996666d111111111144499a7aaa7777777777777777711111111111111
99996666699994444444446666dddddddddddd333dddddddddddddddddddddddddd999996666d1111111111444997a7ab7777777777777777171111111111111
99996666699994444444446666ddddddddddd6333dddddddddddddddddddddddddd999996666d111111111144499a7a7a7ccc777777777777717111111111111
99996666694444444444dd66667dddddddd666111ddddddddddddddddddddddddd9999996666d1111111111444497a7aa72222777777777aaa71711111111111
99996666694444444445dd6666ddddddd66666dddddddddddddddddddddddddddd9999996666d111111666644447a7aaaa2222777777777aaaa7171111111111
999966666944444444dddd6666ddddd6666666ddddddddddddddddddddddddddd99955556666d111111666644ee97aaaaa22eeaaa777777eaaaa711111111111
999966666554444dddddd76666ddddd6666666ddddddddddddddddddddddddddd99955556666d111111dddd44ee9a6addd22eeddd767777eeaaaa11111111111
444466666554444ddddddd6666ddd366666666dddddddddddddddddddddddddd999955556666d111111dddd44ee9222dddd2eedddd22277eeaaaa11711111111
444466666554dddddddddd6666d33311111111dddddddddddddddddddddddddd55595555ddd6d111111dddd52ee222255555ee255552222eeaaaa17177111111
444466666554dddddddddd6666d31111111111dddddddddddddddddddddddddd55595555ddd61111111dddd52ee222254994ee549945222eeaaaa71777711111
444466666dddddd7dddddd6666331111111111113333355ddddddddddddd444555595555ddd61111111dddd5222222259aa95259aa95222222aaaa7777171111
44446ddddddddddddddddd6666111111111111113333355ddddddddddddd444555595555ddd61111111dddd5222222259aa95259aa95222222aaaaa771711111
44446ddddddddddddddddd6666111111111111113333333ddddddddddddd334555533335ddd61111111dddd522224a9549945a549945aaa222aaaaa917111111
4ddddddd7ddddddddddddd6666111111111111111113333ddddddddddddd334555533335ddd61111111dddd547744a995555aaa5555aaaaaaaaaaaa911111111
4dddddddddddddddddd6666666111111111111111333333ddddddddddddd334555533335ddd111111116666574744a999999aaaaaaaaaaaaaaaaaaa999111111
ddddddddddddddddddd6666666111111111111111333333ddddddddddddd334555533335ddd111111116666747474a999999aaaaaaaaaaaaaaaaaaa999111111
7ddddddddddddddd6666666666111111111111111113333ddddddddddddd33333353333dddd111111116666575755a999999aaaaaaaaaaaaaaaaaaa999111171
dddddddddddddddd6666666666111111111111111113333ddddddddddddd33333353333dddd111111116666557555a999999aaaaaaaa9999999aaaa999111717
dddddddddddd66666666666666111111111111111333333ddddddddddddd333333111111111111111116666555555a999999aaaaaaaa9999999aaaa999117171
dddddddddddd66666666666666111111111111111333333ddddddddddddd333333111111111111111116666555555a9444444aaaaaaa9999999aaaa444444717
daaaaaaaaaaaaaaa9999966666111111111111111111333ddddddddddddd311111111111111111111116666555555a9444444a99999999999999999994444171
7777aaaaaaaaaaaa9999966666111111111111111133333ddddddddddddd31111111111111111111111ddddd55555a9444444a99999999999999999994444111
7777aaaaaaaaaaaa9999966666111111111111111133333ddddddddddddd31111111111111111111111ddddd5555559444444a99999999999999999994444517
7777aaaaaaaaaaaa9999944441111111111111113333333ddddddddddddd11111111111111111111111ddddd5555559444444a99999999999999999994444511
7777aaaaaaaaaaaa9999944441111111111113333333355ddddddddddddd55555551111111111111111ddddd5555559444444a99999999999999999994444511
7777aaaaaaaaaaaa9999944445555555111113333333355ddddddddddddd55555551111111111111111ddddd5555559444444a99999999996699999994444555
7777aaaaaaaaaaaa9999944444444445333333333333355ddddddddddddd55555551111111111111111ddddd5555559444444a99999999996969999994444444
7777aaaaaaaaaaaa9999944444444445333333333333355ddddddddddddd55555551111111111111111ddddd7555759444444a99999999444644499994444444
7777aaaaaaaaaaaa9999999444444445555333333335555ddddddddddddd55555551111111111111111111175737379444444a96999999446464499994444444
7777aaaaaa9999999999999444444445555333333335555ddddddddddddd55555551111111111111111666767373737455555569699999444644499994444444
7777aaaaaaaa99999999999444444445555333333335555ddddddddddddd55555551111111111111111667675737379455555596444444444444444444444444
7777aaaaaaaa99999999999444444445555333333555555dddddddddddd555555551111111111111111111717373739455555594644444444444444444444444
aaaaaaaaaaaa99999999999444444445555333333555555dddddddddddd555555551111111111111111111175337339455555596664444444444444444444444
aaaaaaaaaaaa99999999999444444445555333ddd555555dddddddddddd5555555dddddd11111111111111115333339455555596664444444444444444444444
aaaaaaaaaaaa99999999999444444445555333ddd555555dddddddddddd5555555dddddd11111111111111115333333455555594666444444444444444445555
aaaaaaaaaaaa99999999999444444445555333ddd555555dddddddddddd5555555dddddd11111111111111111333333455555594666444444444444444445555
aaaaaaaaaaaa999999999994444444433333335555555555555555555555555555dddddd11111111111111111333333455555596464444444444444444445555
aaaaaaaaaaaa99999999999444445555555333555555555555555555555555555ddddddd11111111111111111333333455555554644444444444444644445555
aaaaaaaaaaaa99999999999444445555555333555555555555555555555555555ddddddd11111111111111111333333455555554444444444444446464445555
aaaaaaaaaaaa99999999999555555555555333555555555555555555555555555dddddd111111111111111111333333455555554444444445555555646445555
aaaaaaaaaaaa99999999999555555555555333555555555555555555555555555dddddd111111111111111111333333455555554444644445555556564445555
aaaaaaaaaaaa99999944444444555555555333555555555555555555555555555dddddd111111111111111111333333455555554446464445555555646445555
aaaaaaaaaaaa99999944444444555555555333555555555555555555555555555dddddd111111111111111111333333355555554464646445555556564645555
aaaaaaaaaaaa44444444444444555555555333555555555555555555555555555dddddd111111111111111111333333355555554456565555555555656555555
aaaaaaaaaaaa44444444444444555555555333555555555555555555555555551111111111111111111111111333333355555554455655555555556565655555
aaaaaaaaaaaa44444444466666555555555333555555555555555555555555551111111111111111111111111333333355555554455555555555555656555555
aaaaaaaaaaaa44444444466666555555555333555555555555555555555555551111111111111111111111111333333355555554455555555555555565555553
aaaaaaaaaaaa44444444466666555555555333555dddddddddddddd5555555551111111111111111111111111333333355555554455555555555555555555553
aaaaaaaaaaaa44444444466666555555555333555dddddddddddddd5555555551111111111111111111111111333333355555554455555555555555555555553
aaaaaaaaaaaa44444444466666555555555111111dddddddddddddd1111111111111111111111111111111117111111155555554455555555555555555555553
aaaaaaaaaaaa44444444466666555555555111111dddddddddddddd1111111111111111111111111111111171711111155333333355555555555555555555553
aaaaa444444444444444466666555555555111111dddddddddddddd1111111111111111111111111111111717173333355333333355555555555555555555553
aaaaa444444444466666666666555555555111111dddddddddddddd1111111111111111111111111111117171737333355333333355555555555555555555553
aaaaa444444444466666666666555555555111111dddddddddddddd1111111111111111111111111111171717173333355333333355333333333333333355553
aaaaa444444444466666666666555551111111111111111111111111111111111111111111111111111117171711111111333333355333333333333333355553
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0281820666650005666665056666650000000000000000000000000049aaa940049aaa9400000000000000000097a94056666650566666505666665056666650
08e88800056600066000660660006600000000000000000000000000124a92000029a42100000000000000000047a92066000660660006606600066066000660
0288820000770007700077077000770000000000000000000000000004a9400484049a4000000000000000000014941077000770770007707700077077000770
00282000006600066000660660006600000000000000000000000000002a924a9129a20000000000000000000002920066000660660006606600066066000660
0002000ddddddd01ddddd101ddddd100000000000000000000000000000149a4249410000000000000000000004999201ddddd101ddddd101ddddd101ddddd10
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000

__gff__
0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0000000003000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
__map__
0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
__sfx__
011000001a3300e1151a3001a0021a00200301013010c201004011800018000180001800018000180001800018000180001800018000180001800018000180001800018000180001800018000180001800018000
01100000306253c6051c5050e5051d5051c505155051d5051f5000e4011640116401174011840118401044010a401104010e4010c401104010e401184010c4011040117401024010c4010e401024010a40110401
01100000246251861418634186511864118631186211861118611186110c6110061500601006050c6000c60002200346053460434605286033460534605346050220034605346043460528603346053460534605
011000000267302661026510264102631026210261102611026150e0001f206272062d505000001d5052d505285051d5052b5051d505295052b50526505295052850526505295052850526505000002850511700
011000001a5631a5131a5031a5031a5031a5031a5031a5031f30526405263051f3052120526305294052120502405024050e4050e40502405024050e4050e40502405024050e4050e40502405024050f4050c405
0110000002264022310221102215032011c305244051f2051f30524405263051f3051f20526305284051f2051a200262051b200000001f200000002420000000282000000024200000001f200000001c20000000
011000081a6011a6011a6011a6011a6011a6011a6011a6011f60124401263051f3051f20526305284051f205022000220002200022000e2000e200266040e2003260302200022000220011200112003260411200
011000001a5031a5051a5031a0031a0031a0031a0031a003024020240202402024020240202402024020240202402024020240202402024020240202402024020240202402024020240202402024020240202402
010c00000024500445003450c4451824500445003450c4450024500445003450c4451824500445003450c4450024500445003450c4451824500445003450c4450024500445003450c44518245004450c34500445
010c00000a2450a4450a34516445222450a4450a345164450a2450a4450a34516445222450a4450a345164450a2450a4450a34516445222450a4450a345164450a2450a4450a34516445222450a445163450a445
010c00000824508445083451444520245084450834514445082450844508345144452024508445083451444508245084450834514445202450844508345144450824508445083451444520245084451434508445
010c0000052450544505345114451d245054450534511445052450544505345114451d245054450534511445072450744507345134451f245074450734513445072450744507345134451f245074451334507445
010c00000c2450c4450c34518445242450c4450c345184450c2450c4450c34518445242450c4450c345184450c2450c4450c34518445242450c4450c345184450c2450c4450c34518445242450c445183450c445
010c000024500245002450024500185001f5001f500295001f50000000205000000000000000001f5000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
010c00200c3350c3351f2250c2353c615306141b335246030c2350c335184350c2353c61524614002350c6140c3350c3350c3050c2353c61524614183350c6140c2350c2351f2250c3353c61524614002350c604
010c00000d3050d305112050d20530603256031b305246030c2050c305184050c205306032460300205032040c3050c305004020c205306032460318305246030c2050c2051f2050c30530603246030020524000
010c001018335274151b335183151f2351b31524435273251f33524415263351f3251f23526315274351f21500000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
__music__
01 10080e4d
00 10090e44
00 100a0e44
00 100b0e44
00 100a0e44
00 10090e44
02 100c0e44
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344

