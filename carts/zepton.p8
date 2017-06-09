pico-8 cartridge // http://www.pico-8.com
version 8
__lua__
-- zepton 1.0
-- a game by rez

-- „todo„
-- drop bonus at kill/score?
-- 

cartdata"zepton"
poke(0x5f2d,1)

local dbg,inv,mse,
	fps,scr,shp,dx,dy,
	f,d,cz,nx,nz,
	u,u2,u3,u4,uh,uz=
		dget"8">0,
		dget"9">0,
		dget"10">0,
		{},{},{},{},{},
		128,512,32,40,48,
		8,16,24,32,4,10.66666--512/48

for i=0,23 do fps[i]=0 end
for i=0,7 do scr[i]=dget(i) end
for i=0,7 do dx[i],dy[i]=0,0 end

local	mode,t,tw,tb,
	tv,tz,tn,az,
	px,py,pz,
	sx,sy,sz,se,sh,
	foe,lsr,er,lr,
	opt,smo,exp,sta,
	bul,bn,bp,bf,
	msl,mn,mp,
	lbod,lbt,
	mt,mz,ms,
	lvl,fc,cs,
	s,si,ix,iy

function _init()
	local n=0
	for k=0,1 do
		for j=0,6 do
			for i=0,6 do
				c=sget(8*k+i,j+16)
				if c!=0 then
					v={
						t=0,c=c,
						x=i-3.5,y=-(k+1),z=j-3.5,
						w=1,h=1}
					if(c==6 or c==9 or c==8) v.t=1
					if(c==14) v.w=0.5 v.x+=0.25
					add(shp,v)
				end
			end
		end
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
	menuitem(4,"button/mouse",
		function()
			mse=not mse
			dset(10,mse and 1 or 0)
		end)
	menuitem(5,"invert y-axis",
		function()
			inv=not inv
			dset(9,inv and 1 or 0)
		end)
	init(1)
end

function init(m)
	-------------------------reset
	mode,
	t,tw,tb,
	tv,tz,tn,
	px,py,pz,
	sx,sy,sz,se,sh,
	foe,lsr,er,lr,
	opt,smo,exp,sta,
	bul,bn,bp,bf,
	msl,mn,mp,
	lbod,lbt,
	mt,mz,ms,
	lvl,fc,cs,
	s,si,ix,iy=
		m,
		0,16,time()*32,
		{},{},0,
		0,0,0,
		0,96,6,100,0,
		{},{},16,128,
		{},{},{},{},
		{},0,0,0,
		{},0,0,
		false,false,
		false,d,0,
		0,0,0,
		0,0,0,0
	--------------------------init
	if mode==0 then
		sc,sn=0,32
		for i=0,sn-1 do
			sta[i]={
				x=flr(rnd(128)),
				y=flr(rnd(48))}
		end
		-------------------init voxel
		for i=0,nz-1 do
			tv[i],tz[i]=voxel(),i*uz
		end
		music(-1)
	else ----------------init menu
		sn=400
		for i=0,sn-1 do
			local v={
				z=256/sn*i,r=32+rnd(480),
				a=rnd(1),t={}}
			local a=v.a+0.25 --cos(0)/4
			local x,y,z=
				v.r*cos(a)+128,
				v.r*sin(a)+128,
				v.z%256
			for j=0,5 do
				v.t[j]=
					{x=x*32/z,y=y*32/z,z=z}
			end
			sta[i]=v
		end
		for i=0,7 do
			for j=0,11,2 do
				local k,b=
					sn+i*12+j,0.0416666 --1/24
				a=0.1666666*i+0.1666666*j
				sta[k],sta[k+1]=
					{x=0,y=0,a=a-b},
					{x=0,y=0,a=a+b}
			end
		end
		music(m==1 and 0 or 8)
	end
end

function voxel()
	az=tn%nz
	local l,py={},
		u2-u2*sin((tn+nz-16)/nz/4)
	local p,bx,rx,cl=
		27,13,35,{7,10,9,4,5,3,3,1}
	for j=0,nx-1 do
		----------------------terrain
		local y=py
		+u2*cos(4/nz*min(az,nz/4))
		+u2*cos(3/nx*j) --x
		+u2*sin((tn-j*4)/nz) --y
		+u3*sin((2/nz)*(tn+j))
		y=flr(min(u2+y,u4)/uh)*uh
		local c=cl[flr(max((u4+y)/8,1))]
		local by,bw,bh,bc=u4,1,u,0
		----------------------volcano
		local vy=-u4
		if y<vy then
			if y>vy-u then
				y,c=vy+u,6
			else --lava!
				y,c,by,bc=vy+u2,8,vy,16
			end
		end
		------------------------river
		if (j==p-1 or j==p+2)
		and y<u4 then
			c=tn%4<2 and 6 or 13
		end
		if(j>p-1 and j<p+2) y,c=u4,1
		-----------------------bridge
		if py<u2 then
			local ry=py
				+cos(2/nz*max(az,nz/2))*u
			if abs(j-bx)<2 then
				y=max(y,ry+u)
				if j==bx-1 then
					by,bw,bc=ry,3,13
				end
				if j==bx and tn%2==0 then
					by,bw,bh,bc=ry,0.25,u/4,7
				end
				if y>ry and tn%8==0 then
					y,c=ry+u,6
				else
					y=max(y,ry)
				end
			end
			if j==bx-2 or j==bx+2 then
				if y>ry and tn%16==0 then
					y,c,by,bc=ry-u2,6,ry-u3,5
				end
				--if y<ry then --wall
				--	c=tn%4<2 and 6 or 13
				--end
			end
		end
		l[j]={
			x=(j-nx/2)*u,y=y,w=1,c=c,
			by=by,bw=bw,bh=bh,bc=bc,
			e=false}
	end
	--------------------------road
	if abs(az-rx)<1.5
	and py<u3 then
		for j=0,nx-1 do
			v=l[j]
			if abs(j-bx)>2
			and v.y<u then --tunnel
				v.by,v.bh,v.bc=
					v.y,u-v.y,v.c
			end
			v.y,v.c=u3,13
			if j>p-2 and j<p+3 then
				v.y,v.c=u4,1 --water
			end
			if j==p-2 or j==p+3 then
				v.y,v.by,v.bh,v.bc=
					u3,u2+uh,uh,5
			end
			if j==p-1 or j==p+2 then
				v.y,v.c,v.by,v.bc=u3,6,u2,5
			end
			if j==p then
				v.by,v.bw,v.bc=u+uh,2,5
			end
		end
	end
	if az==rx-1 or az==rx+2 then
		for j=0,nx-1 do
			v=l[j]
			if v.y<u and abs(p-bx)>1 then
				v.by,v.bh,v.bc=
					v.y,u-v.y,v.c
				v.y,v.c=u,6
			end
		end
	end
	-----------------------objects
	if py<u3 then
		obj(l,16,18,0)  --airport
		obj(l,1,10,16)  --pyramid
	elseif tn>nz then
		obj(l,6,6,32)   --tower
		obj(l,16,18,48) --temple
		obj(l,8,30,64)  --platform 1
		obj(l,22,38,64) --platform 2
	end
	vopt(l)          --optimize
	tn+=1
	return l
end

function obj(l,p,n,o)
	if az>n and az<n+9 then
		for j=0,7 do
			local x,y=j+o+8,n+8-az
			local v,c=l[p+j],sget(j+o,y)
			if c!=0 then
				v.y,v.c=(4-sget(x,y))*u,c
			end
			y+=8
			c=sget(j+o,y)
			if c!=0 then
				v.by,v.bc=(4-sget(x,y))*u,c
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
	if count(msl)<8 then
		local x=mn%2>0 and 1 or -1
		local v={
			x=mx+x,y=my,z=sz,
			vx=x/6,vy=0.166666,vz=0.125,
			t={x=mx,y=my,z=d,
				w=0,h=0,l=false}
			}
		if(mt) v.t=mt
		add(msl,v)
		mn+=1
		sfx(2)
	end
end

function fire()
	local x,y,z=
		bn%2>0 and 2 or -2,
		bn%2>0 and ix8 or -ix8,
		sz+4
	add(bul,{x=mx+x,y=my-y,z=z})
	smoke(mx+x,my-y-1,z,0,13)
	bn,bf=bn+1,2
	sfx(1)
end

function addfoe(i)
	local j,y,w,h=
		flr(4+rnd(nx-8)),
		-(5+rnd(16))*u,
		2+flr(rnd(8+lvl)),
		2+flr(rnd(8+lvl))
	local e={
		e=(w+h)*5,    --energy
		f=(w+h)*5,    --force
		k=false,
		l=false,      --lock
		t=t,          --time
		x=(j-nx/2)*u+uh,y=y,z=tz[i],
		w=w,h=h,
		py=y,
		vy=0.25,
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
	local zf=1/(cz+e.z)*f
	local zu=zf*2
	local x,y,w,h,g=
		(px+e.x)*zf,(py+e.y)*zf,
		e.w*zf-zu-1,e.h*zf-zu-1,
		(py+e.v.y)*zf
	local c={0,1,2,4,8,9,10,7}
	rectfill(x+w,y+zu,x-w,y-zu,0)
	rectfill(x+zu,y+h,x-zu,y-h)
	circfill(x,y,uh*zf)
	circfill(x-w,y,zu)
	circfill(x+w,y,zu)
	circfill(x,y-h,zu)
	circfill(x,y+h,zu)
	if e.lf>0 then
		circfill(x,y,zu,c[flr(e.lf)])
		e.lf-=0.25
	end
	if e.e>0 and zu>1 then
		if t%64<32 and e.w>6 then
			circfill(x-w,y,zu/2,12)
			circfill(x+w,y,zu/2)
		end
		if t%64>31 and e.h>6 then
			circfill(x,y-h,zu/2,12)
			circfill(x,y+h,zu/2)
		end
	end
	if e.e>0 and e.y>-16*u ---bolt
	and t%256>=e.bt
	and t%256<e.bt+e.bl then
		local n,x1,y1=16,x,y+h+zu*2
		line(x,y1,x,g,12)
		circfill(x1,y1,zu/2,7)
		for i=1,n do
			local x2,y2=
				x1+(rnd(4)-2)*zu,
				y1+((e.v.y-e.y)/n*zf)
			line(x1,y1,x2,y2)
			x1,y1=x2,y2
		end
		if t%256>e.bt+e.bl-1 then
			e.bl,e.bt=
				4+rnd(28),
				rnd(256-e.bl)
		end
	end
end

function ec(e,x,y)
	if e then
		local w,h=
			abs(e.x-x),abs(e.y-y)
		return (w<e.w and h<2)
		or (h<e.h and w<2)
		or (w<uh and h<uh)
	end
end

function ex(x,y,z)
	for i=0,15 do
		add(exp,{
			x=x,y=y,z=z,
			vx=(rnd(0.5)-0.25),
			vy=(rnd(0.5)-0.25),
			w=(0.5+rnd(0.25))*u})
	end
	sfx(3,-1,8/d*z)
	cs=2/d*(d-sz-z)
end

function addopt(i)
	local j=flr(3+rnd(nx-6))
	local v=tv[i][j]
	if vc!=1 and v.y<u2 then
		local y=v.y
		local o={
			x=(j-nx/2)*u+uh,
			y=y-y%u-u-uh,
			z=tz[i],
			t=flr(rnd(3)),
			v=v}
		add(opt,o)
		v.o=o
	end
end

function bonus(o)
	local zf=1/(cz+o.z)*f
	local x,y,w,h=
		(px+o.x)*zf,(py+o.y)*zf,
		zf/2,zf*2
	if o.t<2 then
		circfill(x,y,2*h,12)
		rectfill(x-w,y-h,x+w,y+h,7)
		rectfill(x-h,y-w,x+h,y+w)
	else
		circfill(x,y,2*h,8)
		rectfill(x-h,y-zf-w,x+h,y-zf+w,0)
		rectfill(x-h,y+zf-w,x+h,y+zf+w)	
	end
end

function smoke(x,y,z,w,c)
	add(smo,{
		i=0,
		x=x,y=y,z=z,w=w,c=c,
		vx=(rnd(0.0625)-0.03125),
		vy=(rnd(0.0625)-0.03125)})
end

function tc(i,j,y,z)
	local v=tv[i][j]
	if v and z>tz[i] then
		if(y>v.y) return 1
		if y>v.by
		and y<v.by+v.bh then
			return 2
		end
	end
	return 0
end

function tf(j,x,y,z)
	local j=flr((x+nx/2*u)/u)
	for i=pz,nz-1 do
		if(tc(i,j,y,z)!=0) return i
	end
	for i=0,pz-1 do
		if(tc(i,j,y,z)!=0) return i
	end
	return -1
end

function tk(i,j,y,z)
	local v,c=tv[i][j],tc(i,j,y,z)
	if v.w>1 then
		local b=tv[i][j+1]
		b.w,b.y,b.c=v.w-1,v.y,v.c
	end
	if c==1 then
		v.y,v.by,v.bh,v.bc=
			y+u,y+uh,u,0
	elseif c==2 then
		if v.bc==0 then
			v.y,v.by=y+u,y+uh
		else
			v.by=u4
		end
	end
end

function dead()
	se,s,tw=0,0.5,32
	ex(mx,my,sz)
	if(mt) mt.e=0
	for i=0,7 do --update score
		if sc>scr[i] then
			for j=6,i,-1 do
				scr[j+1]=scr[j]
			end
			scr[i]=sc
			for j=0,7 do --save score
				dset(j,scr[j])
			end
			return
		end
	end
end

function game()
	px,py=sx*0.9375,sy*0.9375+u2
	local x,y,z,zf,zu,r,x2,z2
	local a=t/10
	local cx,cy=
		sx/4-64,--+cs*cos(a)
		sy/4-48+cs*sin(a)
	ch=cy+120
	camera(cx,cy)
	clip(0,0,128,121)
	---------------------------sky
	y=u2/d*f
	local w,h,y2,c=cx+127,2,y,
		{15,14,13,5,1}
	for i=0,5 do
		rectfill(w,y2,cx,y2-h-1,c[i])
		rectfill(w,y2+2,cx,y2+2)
		y2-=h
		h*=1.5
	end
	rectfill(w,cy-1,cx,y2)
	-----------------------horizon
	rectfill(w,y-1,cx,y-1,7)
	rectfill(w,y,cx,y,12)
	---------------------------sea
	rectfill(w,y+1,cx,y+1,13)
	rectfill(w,ch,cx,y+2,1)
	-------------------------stars
	c={7,6,13,13}
	for i=0,sn-1 do
		v=sta[i]
		if(i%8==0) color(c[i/8+1])
		pset(
			cx+(v.x-cx+mx/4)%128,
			v.y-56+y)
	end
	-----------------------planets
	x=mx/8
	spr(14,x-8,y-24,2,2)  --moon
	x=mx/6
	spr(42,x-40,y-17,4,2) --sun
	spr(47,x+8,y-32)  --mercury
	spr(46,x+40,y-24) --venus
	x=mx/4
	spr(62,x-84,y-19) --titan
	spr(63,x-48,y-40) --neptune
	-------------------------voxel
	for i=nz-1,0,-1 do
		dt((pz+i)%nz)
	end
	---------------------explosion
	c={7,10,9,8,4,5,13,6}
	for v in all(exp) do
		zf=1/(cz+v.z)*f
		circfill(
			(px+v.x)*zf,
			(py+v.y)*zf,
			v.w*zf,
			c[min(flr(12/v.w),7)])
	end
	------------------------bullet
	for b in all(bul) do
		for i=0,3 do
			zf=1/(cz+b.z+(2-i))*f
			x,y=(px+b.x)*zf,(py+b.y)*zf
			w=0.125*zf
			rectfill(x+w,y+w,x-w,y-w,9)
		end
	end
	-----------------------missile
	c={8,8,5,9}
	for m in all(msl) do
		for i=1,4 do
			zf=1/(cz+m.z+(3-i))*f
			x,y=(px+m.x)*zf,(py+m.y)*zf
			w=0.25*zf
			rectfill(x+w,y+w,x-w,y-w,c[i])
		end
		if m.vz>0.5 then
			zf=1/(cz+m.z-2)*f
			circfill(
				(px+m.x)*zf,
				(py+m.y)*zf,zf,10)
		end
	end
	------------------------smoke1
	for v in all(smo) do
		if v.z>sz then
			ds(v.x,v.y,v.z,v.w,v.c)
		end
	end
	-------------------------laser
	for l in all(lsr) do
		for i=0,3 do
			r=l.r+i*2
			zf=1/(cz+l.z-r*cos(l.a))*f
			circfill(
				(px+l.x+r*sin(l.a))*zf,
				(py+l.y+r*sin(l.b))*zf,
				zf,8)
		end
	end
	--------------------------lbod
	z=ec(mt,mx,my) and mz or d
	zf,z2=1/(cz+sz+2)*f,1/(cz+z)*f
	if lbod and mz>sz and se>0 then
		for i=0,5 do
			r=-0.9+i*0.3
			line(
				(px+mx+r)*zf,(py+my)*zf,
				(px+mx+r)*z2,(py+my)*z2,8)
		end
		sfx(6)
	end
	---------------------spaceship
	if se>0 then
		for v in all(shp) do
			zf=1/(cz+sz-v.z)*f
			zu=0.5*zf
			x,y=
				(v.x+px+mx+v.z*ix8)*zf,
				(v.y+py+my-v.x*ix8+v.z*iy8)*zf
			if v.t==0 then
				w,h=v.w*zf-1,v.h*zf-1
				rectfill(x+w,y+h,x,y,v.c)
			elseif v.c==6 then --canon
				circfill(x+zu,y+zu,zu,6)
			elseif v.c==9 then --reactor
				w=zu-4
				spr(s<0.5 and 35 or 34,x+w,y+w)
			elseif bf>0 then --flame
				w=zu-4
				spr(36+flr(rnd(4)),x+w,y+w,1,1)
				bf-=1
			end
		end
		if sh>0 then ----------shield
			zf=1/(cz+sz)*f
			x,y,w=
				(px+mx)*zf,(py+my)*zf,
				sh*zf
			c={1,12,6,7}
			for i=0,3 do
				circfill(x,y,w-w/4*i,c[i+1])
			end
		end
	end
	------------------------smoke2
	for v in all(smo) do
		if v.z<=sz then
			ds(v.x,v.y,v.z,v.w,v.c)
		end
	end
	------------------------target
	c=11
	if my<-u4 then -----air to air
		if mt and mt.e>0 then
			zf=1/(cz+mt.z)*f
			x,y,w,h=
				(px+mt.x)*zf,
				(py+mt.y)*zf,
				max(0.5,mt.w)*zf,
				max(0.5,mt.h)*zf
			if(mt.l) c=8
			pal(8,c) --lock
			spr(12,x-w-1,y-h-1)
			spr(13,x+w-6,y-h-1)
			spr(28,x-w-1,y+h-6)
			spr(29,x+w-6,y+h-6)
			pal()
			print(flr(mt.z-sz),
				x+w+3,y-2,c)
			print(-flr(-mt.e/50),
				x-w-5,y-2,c)
			v=(w*2+2)/mt.f*mt.e
			rectfill(
				x+w+1,y+h+4,x-w-1,y+h+3,0)
			rectfill(
				x-w-1+v,y+h+4,x-w-1,y+h+3,
				mt.e>mt.f/2 and 12 or 8)
			if mt.l and ms!=1 then
				sfx(0)
				ms=1
			end
		else
			ms=0
		end
	else -----------air to surface
		zf=1/(cz+mz)*f
		zu=zf*u+2
		x,y=
			(px+mx-mx%u)*zf-1,
			(py+my-my%u)*zf-1
		if mz<d then
			rect(x+zu,y+zu,x,y,c)
			print(flr(mz-sz),
				x+zu+2,y+zu/2-2)
		end
	end
	for m in all(msl) do
		v=m.t
		if v.l and v.y>-u4 then
			zf=1/(cz+v.z)*f
			zu=zf*u+2
			x,y=
				(px+v.x-v.x%u)*zf-1,
				(py+v.y-v.y%u)*zf-1
			rectfill(x+zu,y+zu,x,y,8)
		end
	end
	zf=1/(cz+mz)*f
	r,x,y=1,
		(px+mx)*zf,
		(py+my)*zf
	color(ec(mt,mx,my) and 8 or 11)
	if mt and mz!=d and se>0 then
		r=(mt.w+mt.h)*zf
		if not lbod then
			zf=1/(cz+sz+u)*f
			y2=(py+my)*zf
			x2=((px+mx)*zf-x)/(y2-y)
			for i=0,y2-y-1,2 do
				pset(x+x2*i,y+i)
			end
		end
	end
	-----------------------pointer
	for i=0,r/2,2 do
		pset(x-r+i,y)
		pset(x+r-i,y)
		pset(x,y-r+i)
		pset(x,y+r-i)
	end
	if btn(5) or mb==1
	or lbod then
		circ(x,y,r+2)
	end
	---------------------------hud
	camera(0,0)
	rectfill(8,24,4,1,0)
	for i=0,3 do
		pset(5,1+(8*i+py+4)%24,7)
		y=1+(8*i+py)%24
		rectfill(5,y,6,y)
	end
	rectfill(5,13,6,13,8)
	print(flr(sy+u4),10,11,0)
	--print(flr(mx),10,5,0)
	if(se>50) pal(8,11)
	spr(40,111,104,2,2)
	if btn(5) and not btn(4)
	or mb==1 then
		spr(10,111,106,2,1)
	end
	if btn(4) and not btn(5)
	or mb==2 then
		spr(26,mn%2==0 and 120 or 109,106)
	end
	if(lbod) spr(27,115,98)
	pal()
	-------------------------speed
	rectfill(1,24,2,1,0)
	if s>0 then
		rectfill(1,24-11*s,2,24,2)
	end
	-------------------------debug
	--if dbg then
	--	print("lvl="..(lvl+1),103,21)
	--	print("er="..er,103,27)
	--	print("lr="..lr,103,33)
	--	print("foe="..count(foe),1,26)
	--	print("lsr="..count(lsr),1,32)
	--	print("opt="..count(opt),1,38)
	--	print("smo="..count(smo),1,44)
	--end
	---------------------------low
	if (mz<sz+4*uz)
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
			print("pressŽ—",-45,63,0)
		end
	end
	---------------------------bar
	camera(0,-121)
	clip()
	rectfill(127,6,0,0,0)
	spr(48,56,0,2,1) --eagle
	spr(se>0 and 50 or 51,-1)
	nbr(se,7,1,3,100,
		(se==0 or (se<25 and t%64<32)) and -16 or 0)
	spr(54,88)
	nbr(sc,96,1,4,1000,0)
end

function dt(i)
	local zi=tz[i]
	local zf=1/(cz+zi)*f
	local zu=zf*u
	local x,y,w,h
	if zi>d-4*uz then
		x=97+((d-zi)/uz)
		for j=0,15 do
			pal(j,sget(x,80+j))
		end
	end
	for j=0,nx-1 do
		local v=tv[i][j]
		if v.c!=1 then
			if(v.o) bonus(v.o)
			y=(v.y+py)*zf
			if y<ch then
				x=(v.x+px)*zf
				rectfill(
					x+v.w*zu,
					y+(u4-v.y)*zf,x,y,v.c)
				--pset(x+zu/2,y-1,8)
			end
		end
		if v.by<u4 then
			x,y=(px+v.x)*zf,(py+v.by)*zf
			if v.bc<16 then
				if y<ch then
					if(v.bw<1) x+=(1-v.bw)*zu/2
					if(v.bh<1) y+=(1-v.bh)*zf/2
					rectfill(
						x+v.bw*zu,
						y+v.bh*zf,x,y,v.bc)
					--pset(x+zu/2,y-1,8)
					if v.bc==0
					and rnd(1024)<8 then
						smoke(v.x+zu/2,v.by,zi,2,0)
					end
				end
			else
				if v.bc==16 --volcano
				and rnd(1024)<16 then
					smoke(v.x+zu/2,v.by,zi,2,6)
				end
			end
		end
		if(v.e) enemy(v.e)
	end
	pal()
end

function ds(x,y,z,w,c)
	zf=1/(cz+z)*f
	pal(6,c)
	spr(
		64+flr(rnd(2))*16
		+min(w*zf,7),
		(px+x)*zf-3,
		(py+y)*zf-3)
	pal()
end

function upd()
	local x,y,z,w,h,v
	local j=flr((mx+nx/2*u)/u)
	s=0.0078125*(32+sy+u4)-si
	--cz=56-48*sin(time()/16)
	if(si>0)	si-=0.015625
	if(sh>0) sh-=1
	if(cs>0) cs-=0.0625
	if(abs(cs)<0.0625) cs=0
	if(dbg and btn(0,1)) s=0
	-------------------------level
	lvl=min(flr(t/1024),12)
	er,lr=16-lvl,128-lvl*4--*8
	-----------------------terrain
	for i=0,nz-1 do
		tz[i]-=s
		if tz[i]<s then
			tz[i]+=d
			pz+=1
			if(pz>nz-1) pz=0
			tv[i]=voxel()
			if(pz%er==0) addfoe(i)
			if(pz%4==0) addopt(i)
		end
	end
	-------------------------smoke
	if se>0 and s>0.5 then
		smoke(
			mx+(t%4<2 and 1 or -1),
			my+(t%4<2 and ix8 or -ix8)-0.5,
			sz-uh,0.125,6+flr(rnd(2))*7)
	end
	if se<50 and se>1
	and t%flr(se/5)==0 then
		smoke(mx+(3-rnd(6)),
			my,sz,0.125,0)
	end
	if t%4<2 then
		for m in all(msl) do
			if m.vz>0.5 then
				smoke(m.x,m.y,m.z-2,
					1,8-rnd(2))
			end
		end
	end
	for v in all(smo) do
		v.i+=0.0625 --1/16
		v.x+=v.vx+v.i/12
		v.y+=v.vy-v.i/8
		v.z-=s
		if v.i<1 then
			v.w+=0.03125 --1/32
		else
			v.w-=0.015625 --1/64
		end
		if v.z<4-cz or v.w<0 then
			del(smo,v)
		end
	end
	-------------------------enemy
	for e in all(foe) do
		e.z-=s
		if(e.z<0) del(foe,e)
		if e.e<1 then
			if(mt==e) mt=false
			if t%4<2 then
				smoke(
					e.x+rnd(e.w*2)-e.w,
					e.y,e.z,1,0)
			end
			e.y+=e.vy
			e.vy*=e.ry
			if e.y>e.v.y then --shutdown
				e.k=true
				v=e.v
				v.by,v.bc,v.bt,v.e=
					v.y,0,0,false --no ref?
				del(foe,e)
				ex(e.x,v.y,e.z)
			end
		end
		if s>0 and e.z>sz
		and flr(t-e.t)%lr==0 then
			z=e.z-sz-4*uz*s
			add(lsr,{
				x=e.x,y=e.y,z=e.z,
				a=0.25+atan2(e.x+sx,z),
				b=0.25+atan2(e.y+sy,z),
				r=0})
			sfx(4)
			e.lf=7
		end
	end
	-------------------------laser
	for l in all(lsr) do
		l.z-=s
		l.r+=4
		x,y,z=
			l.x+l.r*sin(l.a),
			l.y+l.r*sin(l.b),
			l.z-l.r*cos(l.a)
		if se>0 and z<sz+2
		and abs(mx-x)<2
		and abs(my-y)<2 then
			del(lsr,l)
			se-=5
			si+=s*0.5
			s,sh,cs=0.125,8,1.5
			sfx(5)
			if(se<1) dead()
		end
		if(l.z<0) del(lsr,l)
	end
	---------------------spaceship
	if se>0 then
		for e in all(foe) do
			if ec(e,mx,my)
			and abs(e.z-sz)<2 then
				e.e=0
				dead()
			end
		end
	end
	------------------------target
	mz=d
	if(mt and mt.z<sz) mt=false
	if my<-u4 then -----air to air
		for e in all(foe) do
			w,h=abs(e.x-mx),abs(e.y-my)
			if e.z>sz
			and w<e.w+8 and h<e.h+8 then
				mt,mz=e,e.z
				if w<e.w and h<e.h then
					mt.l=true
				end
				break
			end
		end
	else -----------air to surface
		for i=pz,nz-1 do
			v=tv[i][j]
			if my>v.y or (my>v.by
			and my<v.by+v.bh) then
				mz=tz[i]
				goto me
			end
		end
		for i=0,pz-1 do
			v=tv[i][j]
			if my>v.y or (my>v.by
			and my<v.by+v.bh) then
				mz=tz[i]
				goto me
			end
		end
		::me::
		if mz<d then
			mt={
				x=mx,y=my,z=mz,w=uh,h=uh,
				e=0,l=true}
			if abs(mz-sz+uz)<uz
			and se>0 then dead() end
		end
	end
	------------------------bullet
	for b in all(bul) do
		b.z+=uz-s
		if b.y<-u4 then --air to air
			for e in all(foe) do
				if b.z>e.z
				and ec(e,b.x,b.y) then
					del(bul,b)
					smoke(b.x,b.y,e.z,2,0)
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
			local cti=tf(j,b.x,b.y,b.z)
			if cti!=-1 then
				del(bul,b)
				smoke(b.x,b.y,b.z,2,5)
				tk(cti,j,b.y,b.z)
			end
		end
		if(b.z>d) del(bul,b)
	end
	-----------------------missile
	for m in all(msl) do
		local v,x,y=m.t,0,0
		if v.l then
			z=m.vz/32
			x,y=(v.x-m.x)*z,(v.y-m.y)*z
		end
		m.x+=m.vx+x
		m.y+=m.vy+y
		m.z+=m.vz
		m.vz*=1.05
		if(v.y>-u4) v.z-=s
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
			local cti=tf(j,m.x,m.y,m.z)
			if cti!=-1 then
				del(msl,m)
				ex(m.x,m.y,m.z)
				tk(cti,j,m.y,m.z)
			end
		end
		if(m.z>d) del(msl,m)
	end
	--------------------------lbod
	if lbod and se>0 then
		if((t-lbt)%64==0) se-=1
		if mz!=d then
			if my<-u4 then --air to air
				for e in all(foe) do
					if mz+uz>e.z
					and ec(e,mx,my) then
						if e.e>0 then
							e.e=0
							ex(e.x,e.y,e.z)
							sc+=flr(e.f/5)
						end
					end
				end			
			else ---------air to surface
				z=mz+uz
				local cti=tf(j,mx,my,z)
				if cti!=-1 then
					smoke(mx,my,z,2,0)
					tk(cti,j,my,z)
				end
			end
		end
	end
	-------------------------bonus
	for o in all(opt) do
		o.z-=s
		if o.z<-cz then
			del(opt,o)
		else
			if abs(o.x-mx)<uh
			and abs(o.y-my)<uh then
				mz=o.z
				if abs(o.z-sz)<2 then
					if o.t<2 then
						sfx(7)
						if(se>0) se+=10
					else
						sfx(13)
						for e in all(foe) do
							e.e=0
							ex(e.x,e.y,e.z)
							sc+=flr(e.f/5)
						end
					end
					o.v.o=false --no ref?
					del(opt,o)
					mz=d
				end
			end
		end
	end
	---------------------explosion
	for v in all(exp) do
		v.x+=v.vx
		v.y+=v.vy
		v.z-=s
		v.w-=0.125
		if(v.z<0 or v.w<0) del(exp,v)
	end
end

function menu()
	local n,c,c1,c2
	camera(-64,-64)
	rectfill(63,63,-64,-64,0)
	print("pressŽ—",-63,58,1)
	a=t/1080
	sf()
	if mode==1 then ----------logo
		spr(72,25,57,5,1)
		camera(-16,-55)
		n=flr(t/4)%128
		for j=0,15 do
			c1,c2=
				sget(n+j,64),
				sget(n-j,65)
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
			if c1!=0 then
				for i=0,67 do
					if sget(i,72+j)==11 then
						pset(i,j,c1)
					end
				end
			end
		end
	else --------------------score
		for i=0,7 do tri(i) end
		camera(-44,-34)
		spr(107,0,0,5,1)
		for i=0,7 do
			local y=12+i*6
			spr(122,-8,y)
			nbr(i+1,0,y,1,1,0)
			spr(123,8,y)
			nbr(scr[i],16,y,4,1000,
				(sc!=0 and sc==scr[i]
				and t%64<32) and -16 or 0)
		end
	end
end

function sf() --------starfield
	local c,a0,a1,a2,a3=
		{1,2,14,15,7,8,1,5,13,12,7,8},
		1024*sin(a/3),cos(a)/4,
		128*cos(a),128*cos(a/2)
	for i=0,sn-1 do
		local v=sta[i]
		local b=v.a+a1--+(sx+sy)/720
		local x,y,z=
			v.r*cos(b)+a2,
			v.r*sin(b)+a3,
			(v.z+a0)%256
		local zf=32/z
		x,y=x*zf,y*zf
		v.t[0]={x=x,y=y,z=z}
		for j=5,1,-1 do
			v.t[j]=v.t[j-1]
		end
		local p1,p2=v.t[0],v.t[5]
		if abs(p1.x-p2.x)<64
		and abs(p1.y-p2.y)<64
		and abs(p1.z-p2.z)<200
		and abs(x)<80
		and abs(y)<80 then
			local k=6-flr(z/48)
			local n=i%2*6+k
			if k>2 then
				for j=k-1,1,-1 do
					p1,p2=v.t[j],v.t[j-1]
					line(p1.x,p1.y,
						p2.x,p2.y,c[n-j])
				end
			else
				pset(x,y,c[n])
			end
		end
	end
end

function tri(i)
	local x2,y2,c,z={},{},
		{8,8,8,8,8,2,1,1},
		(256-i*32+1024*sin(a/3))%256
	local zf=32/z
	for j=0,11 do
		local v=sta[sn+i*12+j]
		local r,b=
			(j<6) and 20 or 28,
			v.a+cos(a)/4
		local x,y=
			r*cos(b)+128*cos(a),
			r*sin(b)+128*cos(a/2)
		x2[j],y2[j]=x*zf,y*zf
	end
	line(x2[6],y2[6],
		x2[11],y2[11],c[flr(z/32)])
	for j=1,5 do
		if j%2==1 then
			line(x2[j-1],y2[j-1],
				x2[j],y2[j])
		end
		line(x2[j+5],y2[j+5],
			x2[j+6],y2[j+6])
	end
end

function nbr(n,x,y,l,k,o)
	for i=0,l-1 do
		spr(112+o+((n/k)%10),x+i*8,y)
		k/=10
	end
end

function _draw()
	if(mode==0) game() else menu()
	fc+=1
	if dbg then
		camera(0,0)
		--------------------------fps
		local n=flr(stat(1)*25)
		fps[fc%24]=max(0,n)
		rectfill(126,1,103,16,0)
		print(fps[fc%24],104,2,1)
		rectfill(103,8,126,8,2)
		for i=0,23 do
			local v=fps[(i+fc%24+1)%24]
			if v>0 then
				local x,y=103+i,17-v*0.16
				line(x,y,x,16,1)
				pset(x,y,v>50 and 8 or 12)
			end
		end
		--------------------------mem
		n=stat(0)*0.0234375 --24/1024
		rectfill(126,18,103,19,0)
		rectfill(102+n,18,103,19,2)
		--print(stat(0),104,21)
	end
end

function _update60()
	t=flr(time()*32-tb)
	if(mode==0) upd()
	if(tw>0) tw-=1
	-------------------------input
	local xm,ym=17*u,25*u--x/y max
	mb=stat(34)
	if mse then
		sx,sy=
			stat(32)*2.125,
			stat(33)*1.75
		dx[0],dy[0]=sx,sy
		for j=7,1,-1 do
			dx[j],dy[j]=dx[j-1],dy[j-1]
			sx+=dx[j]
			sy+=dy[j]
		end
		sx,sy=xm-sx/8,ym-sy/8
		if(sx<-xm) sx=-xm
		if(sx>xm) sx=xm
		ix,iy=
			(dx[7]-dx[0])/32,
			(dy[7]-dy[0])/16
	else
		if(btn(0) and ix<4) ix+=0.25
		if(btn(1) and ix>-4) ix-=0.25
		if btn(inv and 2 or 3)
		and iy<4 then iy+=0.25 end
		if btn(inv and 3 or 2)
		and iy>-4 then iy-=0.25 end
		---------------------position
		if(ix>0) ix-=0.125
		if ix>1 and sx>xm-u3 then
			ix-=0.5
		end
		if(ix<0) ix+=0.125
		if ix<-1 and sx<u3-xm then
			ix+=0.5
		end
		if(iy>0) iy-=0.125
		if iy>1 and sy>ym-u3 then
			iy-=0.5
		end
		if(iy<0) iy+=0.125
		if iy<-1 and sy<0 then
			iy+=0.5
		end
		if(abs(ix)<0.125) ix=0
		if(abs(iy)<0.125) iy=0
		if abs(sx+ix)<xm then
			sx+=ix
		else
		 ix=0
		end
		if sy+iy>-u3 and sy+iy<ym then
			sy+=iy
		else
			iy=0
		end
	end
	mx,my=-sx,-sy
	ix8,iy8=ix/8,iy/8
	------------------------button
	if mode==0 and se>0 then
		if (btn(4) and not btn(5))
		or mb==2 then
			if(mp%32==0) aam()
			mp+=1
		else
			mp=0
		end
		if (btn(5) and not btn(4))
		or mb==1 then
			if(bp%3==0) fire()
			bp+=1
		else
			bp=0
		end
		lbod=(btn(4) and btn(5))
		or mb>2
		if lbod and not lbt then
			lbt=t
			se-=1
		end
		if(not lbod) lbt=false
	end
	if (btnp(4) or btnp(5) or mb>0)
	and tw==0 then
		if(mode==0 and se<1) init(2) return
		if(mode==1) init(0) return
		if(mode==2) init(1) return
	end
	if btnp(4,1) then --debug
		dbg=not dbg
		dset(8,dbg and 1 or 0)
	end
end
__gfx__
5550000033300000222222221111111100000000000000000dd22dd0022332200000000000000000080000000000008008800000000008800000001212200000
5d5ddd00373222002eeeeee212222221000dd00000011000dd5555dd222222220500050003000300080000000000008080000000000000080000d11111128000
555555d0333222202e2222e2123333210055550000222200d55ee55d222442220000000000000000000000000000000080000000000000080006511111114900
0d55755d022222222e2ee2e2123443210d5dd5d00127721025effe5232477423000000000000000000000000000000000000000000000000006d111111111480
0d57775d022222222e2ee2e2123443210d5dd5d00127721025effe52324774230000000000000000080000000000008000000000000000000d65111111111120
0d55755d022222222e2222e2123333210055550000222200d55ee55d222442220500050003000300080000000000008000000000000000000661111111111112
00d555d0002222202ee55ee212222221000dd00000011000dd5555dd222222220000000000000000000000000000000000000000000000005661111111111112
000ddd0000022200222222221111111100000000000000000dd22dd002233220000000000000000000000000000000000000000000000000d665111111111111
0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000677d555555555555
0b0000000600000000000000000000000000000000000000000220000003300000ddd00000333000000080000008800000000000000000005666111111111111
0000000000000000000000000000000000dddd0000777700002ee200003663000dc5cd000333330000080800000000000000000000000000077765555555555d
0000000000000000000ff0000005500000dccd000078870002eeee20036886300d5c5d0003333300000808000008800000000000000000000d77765555555560
0000000000000000000ff0000005500000dccd000078870002eeee20036886300dc5cd0003333300000808000000000000000000000000000067776d555d6600
0000000000000000000000000000000000dddd0000777700002ee2000036630000ddd00000333000000808000008800080000000000000080006777777776000
00000000000000000002200000033000000000000000000000022000000330000000000000000000000888000000000080000000000000080000d677776d0000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000008800000000008800000000000000000
00000000000000000000000000000000000090000000090090000009009000000000000000000000000000000000000000000000000000000000000000000000
080208000000000000555500005555000009a00000009a000a9009a000a9000000000008800000000000000000000000000000000000000000dccd0000012100
06020600000c000005499450055115500009a0009a99a90009a99a90009a99a90000008008000000000000000000000000000000000000000dccccd000149410
06d2d60000020000059aa950051111509aaaa99009aaa900009aa900009aaa900000008008000000000000000000000000000000000000000cccccc00029a920
02d2d20000000000059aa95005111150099aaaa9009aaa90009aa90009aaa9000000008008000000000000000000000000000000000000000cccccc000149410
22525220e00e00e00549945005511550000a9000009a99a909a99a909a99a9000000008008000000000000000000000000000000000000000dccccd000012100
20909020e00e00e00055550000555500000a900000a900000a9009a000009a000000880000880000000000000000eeeeeeee00000000000000dccd0000000000
00000000000000000000000000000000000900000090000090000009000009000008000000008000000000000eeeeeeeeeeeeee0000000000000000000000000
000000000000000000000000000000000000000000000000000000000000000000080000000080000000000ffffffffffffffffff00000000000000012100000
49aaa940049aaa94002818200057765000018100000000000097a940000000000080000000000800000000eeeeeeeeeeeeeeeeeeee0000000055550028200000
124a92000029a421008e888000766d6000028200000282000047a92000000000080000000000008000000ffffffffffffffffffffff00000053bb35012100000
04a9400484049a40002888200070d060000282000008a800001494100000000080000000000000080000ffffffffffffffffffffffff00000dbbbbd000000000
002a924a9129a2000002820000570650000888000002820000029200000000008000000000000008000ffffffffffffffffffffffffff00005bbbb5000000000
000149a42494100000002000000676000002a2000000000000499920000000008888008888008888000777777777777777777777777770000d3bb3d0000002d2
0000000000000000000000000000000000000000000000000000000000000000000800800800800000ffffffffffffffffffffffffffff0000dddd0000000d6d
000000000000000000000000000000000000000000000000000000000000000000008800008800000077777777777777777777777777770000000000000002d2
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
28888820888820008888882088888820880008808888888028888880888888802888882028888820000000005ddddd505ddddd505ddddd505ddddd505ddddd50
8800088000288000000008800000088088200880880000008800000000000880880008808800088000000000ee200000ee000000ee000ee0ee000ee0ee000000
99000990000990002999992000099920299999909999992099999920002999202999992029999990000000002efffe20ff200000ff200ff0ff20ef20ff22ef20
880008800008800088000000000008800000088000000880880008800088200088000880000008800000000000002ee0eee00000eee00ee0eee02ee0eee00000
14444410444444404444444044444410000004404444441014444410004400001444441044444410000000009aaaa94049aaaa9049aaa9409aa00a9049aaaa90
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
0bbb000000bbbb00bbb00b000b00bbbb0000bbbb00b000b0000bbbb000bbbb0bbbbb000000ddd000005550000000000000000000000000000000000000000000
b000b0000b00000b000b0bb0bb0b00000000b000b00b0b00000b000b0b00000000b000000dcccd0005ddd5000000000000000000000000000000000000000000
bbbbb0000b0bbb0bbbbb0b0b0b0bbb000000bbbb0000b000000bbbb00bbb00000b000000dcc7ccd05d777d500000000000000000000000000000000000000000
b000b0000b000b0b000b0b000b0b00000000b000b000b000000b000b0b000000b0000000dc777cd05d878d500000000000000000000000000000000000000000
b000b00000bbbb0b000b0b000b00bbbb0000bbbb0000b000000b000b00bbbb0bbbbb0000dcc7ccd05d777d500000000000000000000000000000000000000000
0000000000000000000000000000000000000000000000000000000000000000000000000dcccd0005ddd5000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000ddd000005550000000000000000000000000000000000000000000
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
11111111111111111111111100000001d11111111111111111111111111111111111111111112111111111111111111111711111111111111111111111111111
10010000011111111111111110000011111111111111111111111111111111111111111111149411111111111111111111111111111111111111111111111111
1001000001111111111111111000001111111111111111111111111111111111111111111129a921111111111111111111111111111111111111111111111111
10010700011111111111111110000011111111111111111111111111111111111111111111149411111111111111111111111111111111111111111111111111
10010000011111111111111111000111111111111111111111111111111111111111111111112111111111111111111111111111111111111111111111111111
50050000055555555555555555555555555555555555555555555555555555555555555555555555555555555555555565555555555555555555555555555555
10010000011111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111116111111111111111
50050770055555555555555555555555555555555555555555555555555555121225555555555555555555555555555555555555555565555555555555555555
5005000005555555555555555555555555555555555555555d5555555555d111111285555555555555555555555555555555555555dccd555555555555555555
500500000555555555555555555555555555555555555555555555555556511111114955555555555555555555555555555555555dccccd55555555555555555
50050000055555555555555555555555555555555555555555555555556d111111111485555555555555555555555555555555555cccccc55555555555555555
522507000500550055055555555555555555555555555555555575555d65111111111125555555555555555555555555555555555cccccc55555555555555555
522500000550555055055555555555555555555555555555555555555661111111111112555555555555555555555555555555555dccccd55555565555555555
5225088005505550550005555555555555555555555555555555555556611111111111125555555555555555555555555555555555dccd555555555555555555
52250000055055505505055555555555555555555555555555555555d66511111111111155555555555555755555555555555555555555555555555555555555
d22d07700d000d000d000ddddddddddddddddddddddddddddddddddd677d555555555555dddddddddddddddddddddddddddddddddd6ddddddddddddddddddddd
522500000555555555d5555555555555555555555555555555555555566611111111111155555555555555555555555555555555555555555555555555555555
d22d00000dddddddddddddddddddddddddddddddddddddddddddddddd77765555555555dddd00ddddddddddddddddddddddddddddddddddddddddddddddddddd
d22d00000ddddddddddddddddddddddddddddddddddddddddddddddddd7776555555556dddd000dddddddddddddddddddddddddddddddddddddddddddddddddd
d22d07000ddddddddddddddddddddddddddddddddddddddddddddddddd67776d555d66ddddd000dddd0ddddddddddddddddddd6ddddddddddddddddddddddddd
d22d00000dddddddddddddddddddddddddddeeeeeeeeddddddddddddddd67777777767ddddd000ddd000dddddddddddddddddddddddddddddddddddddddddddd
d22d00000ddddddddddddd7ddddddddddeeeeeeeeeeeeeedddddddddddddd677776dddddddddddddd000dddddddddddddddddddddddddddddddddddddddddddd
e22e00000eeeeeeeeeeeeeeeeeeeeeeffffffffffffffffffeeeeeeeeeeeeeeeeeeeeeeeeeee7eeeeeeeeeeeeeeee6deeeeeeeeeeeed6eeeeeeeeeeeeeeeeeee
d22d07700dddddddddddddddddddddeeeeeeeeeeeeeeeeeeeedddddddddddddddddddddddddd7ddddd8ddddddddddddddddddddddddddddddddddddddddddddd
e22e00000eeeeeeeeeeeeeeeeeeeeffffffffffffffffffffffeeeeeeeeeeeeeeeeeeeeeeeee7eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee
eeeeeeeeeeeeeeeeeeeeeeeeeeeeffffffffffffffffffffffffeeeeeeeeeeeeeeeeeeeeeee7ceeeeeeeeeeeeeeeeeeeeeeeeeeeee6eeeeeeeeeeeeeeeeeeeee
eeeeeeeeeeeeeeeeeeeeeeeeeeeffffffffffffffffffffffffffeeeeeeeeeeeeeeeeeeeeee7ceeeeeeeeeeeeeeeeeeeeeeeeeeee6eeeeeeeeeeeeeeeeeeeeee
fffffffffffffffffffffffffff77777777777777777777777777ffffffffffffffffffffff7cfffffffffffffffffffffffffffffffffffffffffffffffffff
eeeeeeeeeeeeeeeeeeeeeeeeeeffffffffffffffffffffffffffffeeeeeeeeeebeeeeeeeeee7ceeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee
ffffffffffffffffffffffffff7777777777777777777777777777fffffffffbfbffffffff7fcffffffffffffffffffff6ffffffffffffffffffffffffffffff
7777777777777777777777777777777777777777777777777777777777777777b77777777777c777777777777777777777777777777777777777777777777777
cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc7ccccccccccccccccccccccccccccccccccccccccccccccccccccc
ddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd7ddcddddddddddddddddddddddddddddddddddddddddddddddddddd
1111111111111111111111111111111111111111111111111111111111111111111111111711c111111111111111111111111111111111111111111111111111
1111111111111111111111111111111111111111111111111111111111111111111111111171c111111111111111111111111111111111111111111111111111
1111111111111111111111111111111111111111111111111111111111111111111111111171c111111111111111111111111111111111111111111111111111
1111111111111111111111111111111111111111111111111111111111111111111111111117c111111111111111111611111111111111111111111111111111
1111111111111111111111111111111111111111111111111111111111111111111111111117c111111111111111111111111111111111111111111111111111
11111111111111111111111111111111111111111111111111111111111111111111111111117111111111111111111111111111111111111111111111111111
11111111111111111111111111111111111111111111111111111111111111111111111111117111111111111161111111111111111111111111111111111111
1111111111111111111111111111111111111111111111111111111111111111111111111111c711111111111111111111111111111111111111111111111111
1111111111111111111111111111111111111111111111111111111111111111111111111111c711111111111111111111111111111111111111111111111111
1111111111111111111111111111111111111111111111111111111111111111111111111111c711111111111111111111111111111111111111111111111111
11111111111111111111111111111111111111111111111111111111111111111111111111117111111111111111111111111111111111111111111111111111
11111111111111111111111111111111111111111111111111111111111111111111111111117111111111111111111111111111111111111111111111111111
11111111111111111111111111111111111111111111555511111555111144ddd11111111117c111111111111111111111111111111111111111111111111111
1111111111111111111111111111111111111111111155551111155511119999911111111117c111111111111111111111111111111111111111111111111111
1111111111111111111111111111111111111111111155551111155511119aaaa99111111117c111111111111111111111111111111111111111111111111111
11111111111111111111111111111111111111111111666655555666114aaaaaaaa111111171c111111611111111111111111111111111111111111111111111
11111111111111111111111111111111111111111111666655555666ddd99aaa999111111171c111111111111111111111111111111111111111111111111111
111111111111111111111111111111111111111111116666ddddd666ddd99999999441111171c111111111111111111111111111111111111111111111111111
1111111111111111111111111aaaaaaa911111111111666dd77dd666bbb99999999441111171c111111111111111111111111111111111111111111111111111
111111111111111111111111aaaaaa9991111111111166dddddddd66bbb99444444441111171c111111111111111111111111111111111111111111111111111
11111111111111111111111aaaaaa99999111115555566ddd7dddd66bbb44444444441111171c555556111555551111111111111111111111111111111111111
11111111111111111111119aaaaa999999441555555566ddd7dddd66bbb44444445555511175c555565651555555511111111111111111111111111111111111
111111111111111111111999999999999944155555556dddddddd666ddd445ccc55555511755c555556555555555555111155511111111111111111111111111
111111111111111111111999999999944444555555556ddd77ddd666ddd55522255555555755c55555556ddd5555555555555555511111111111111111111111
11111111111111111114499999999944444453ddddddddddddddee66ddd5552e255533333ee5c5511d5d6ddd444dddddddddddddd11111111111111111111111
1111111111111111111449999999944444445333ddddddd77dddee55ddd5552e233533333eedc6dd616d6d6664445dddddddddddd11111111111111111111111
1111111111111111114449444444444444555533ddddddd5555dee55d65ddd2e2ddd33633ee3c6d6d66d6d666445555dddddddddd11119999aaaa11111111111
1111111111111111114444444444444445555531111ddd75555dee522255552e255552223ee6c6dd6d1d6d666555555551111144444999999aaaaaa911111111
111111111111111111444444444444445555551111ddddd55552ee222549945e549945222ee267d6661d6d66655555555555444444999999999aaa9999111111
111111111111111113444444444444455555553111ddd77555522222259aa95259aa9522222277d6661d6d6ddd55555555554444449999999999999999991111
11111111111111111555544444455555556666311dddddd666622222259aa95259aa9522222277d666116d6ddd55555555554444449444999999999999991111
111111111ffffffff55554444445555555666611dddd77d66662225dd5499457549945767222773666677777ddd5555555554444449444499999999999994111
1111111fffffffff777555555555555555666631ddddddd66666155ddd555555755557676777777777677777ddd5553335555445559444499999999999999911
1111111fffffffff77755555555555555566663dddd77dd66666155ddd555555777777767777777777d7777777d5553335555445555444499999999999999941
1111111ffffffffffc33555555555555556666ddddddddd66666155dddddd555777d7777777776666677777777d5333335555445555444444999999999999941
1111111ffffffffee333555553333333356666ddddddddd66665355ddddddddd7777777777777666667777777766333333553335555444444999999999999941
1111111ffffffffee22222555333333335666ddddd7dddd66665311ddddddddd7777677777777666667777777766333333553335555544444999944499994444
1111122eeeeeeeee222222333333333335666ddddd7dddd66665311ddddddddd77777d7777777666667777777766333333553335555544455559944499994444
1111122eeeeeeeee222222eeee3333333366ddddddddddd666633333ddd7d7777777d7d888877777767777777776333333333333555544455559944499994444
1112222eeeeeee2222222eeeee223333336ddddd77ddddd55555553333dd7d7777777d8888877777767777777771133333333333553333455559944499944444
1112222eeeeeee2222222eeeee223333336ddddd77ddddd555555533333dd7777777778888877777767777777771133333333333553333455559944499944444
1222222222222222222eeeeee222333333ddddddddddddd55555533333d7d777777777777777777776666777777aa11113333333353333455555544555544444
1222222222222222222eeeee2223333333ddddddddddddd555555333333d7777777777777777777776666777777aa11111111333353333455555544555544444
e2222222222222222eeeeeee222333333ddddd77dddddd333333333355aaaaaa777777677777777777777777777aaaa111111333353333355555544555544555
e2222222222222222eeeee22222333333ddddd77dddddd333333333355aaaaaa777776767777777777777777777aaaa111111133353333355555544555544555
e222222222222222eeeeee2222233333dddddddddddddd333333333355adaaaa77777d677777777777777777777aaaa111111111113333355553333555555555
e222222222222222eeeee22222333333ddddddddddddd3333333333355dadaaa7777d7d77777777777777777777aaaa11111111111333333355333ccc5555555
ee22222222eeeeeeeeeee222223333dddddd77ddddddd3333333333444adddaa77777d777777777777777777777aaaa1111111111111333335533cc7cc555555
9999999995eeeeeeeee22222223333dddddd77ddddddd3333333334444adddaa777777777777777777777777777aaaa1111111111111333335533c777c559999
aaaaaa9995eeeeeeeee22222211133ddddddddddddddd3333333334444dadaaaaaaaa77777777777777777aaaaaaaaaa111111111111113335533c777c55999a
aaaaaa9995eeeeeeee22222221113ddddddddddddddd33333333334444adaaaaaaaaa7d777777777777777aaaaaaaaaadd11111111111133333333c7c554999a
aaaaa999999999eeee22222211111ddddddddddddddd33333333334444aaaaaaaaaaad7d77777777777777aaaaaaaaaadddd111111111113333333333334999a
aaaaa999999999eeee222222111dddddd77dddddddd333333333334444aaaaaaaaaaa7d7d7777777777777aaaaaaaaaadddd111111111113333333333334999a
aaaaa9999999994442222211111dddddd77dddddddd333333333344444aaaaaaaaaaa77d7777aaaaaaa777aaaaaaaaaadddd111111111111111113344444999a
aaaaa9999999994442222211111dddddd77dddddddd333333333344444aaaaaaaaaaa7767777aaaaaaa777aaaaaaaaaadddd111111111111111113344499999a
a99999999999994455555111111ddddddddddddddd1133333333344444aaaa6aaaaaa7676777aaaaaaa777aaaaaaaaaadddd1111111111111111113444999999
a999999999999444444551111ddddddddddddddddd1133333333344444aaa6a6aaaaa6a6a6aaaaaaaaaaaaaaaaaaaaaadddd1111111111111111113444999999
a999999999999444444553311ddddddddddddddddd1133333333344444aa6a6a6aaaa76a6aaaaaaaaaaaaaaaaaaaaaa666666611111111111111113444999999
a999999999999444444553333dddddddddddddddd61111133333344444ada6a6aaaaa7a6aaaaaaaaaaaaaaaaaaaa999999666611111111111155555444999999
99999999994444444445533ddddddd77ddddddddd61111133333344444dcdc6aaaaaa7aaaaaaaaaaaaaaaaaaaaaa999999666611111111111155544444999999
99999999994444444445333ddddddd77ddddddddd61111333333344444cdcdcdaaaaa7aaaaaaaaaaaaaaaaaaaaaa999999666611111111111155544444999999
99999999994444444455555dddddddddddddddd666111333333334444cccd7dcaaaaa7aaaaaaaaaaaaaaaaaaaaaa999999666611111111111155544444999999
99999999994444444455555dddddddddddddddd666111333333335555cc77d7c999999aaaaaaaaaaaaaaaaaa9999999999666611111111133355544444999999
999999944444444444555dddddddddddddddddd666111333333335555cc7777c999999aaaaaaaaaaaaaaaaaa999999999966661111111ddddd55544444444499
999999944444444444555dddddddddddddddddd6661113333333355555cc77c4999999aaaaaaadaaaaaaaaaa999999999966661111111ddddd55544444444499
999999944444444444555ddddddddddddddddd666611133333333555554ccc44999999aaaaaadadaaaaaaaaa9999999999666611111dddd66666644444444499
999999944444444444555ddddddddddddddddd66661113333333355555444444999999aaaaadadadaaaaaaaa9999999996666666111dddd66666644444444499
999999944444444433dddddddd777ddddddddd11111133333333355555444444999999aaaadadadaaaaaaaaa9999999996666666111dddd66666644444444499
555555444444444433dddddddd755555555ddd11111133333333355555444444999999aaaaadadadaaaaaaaa9999999996666666111dddd66666644444444444
555555444444444433ddddddddd55555555d1111111133333333355555444444999999444444d4d4d444aaaa9999999996666666111dddd66666666644444444
555555444444444433ddddddddd55555555d11111111333333333555554644449999994444444d4d4d44aaaa9999999996666666111dddd66666666644444444
555555444444444433ddddddddd55555555d111111111111333335555564644499999944444444d4d4d4aaaa999999555555566611111dd66666666644444444
5555554444444333ddddddddddd55555555d1111111111113333333336464644999999444444444d4d444444449999555555566611111dd6666666bb44444444
5555554444444333ddddddddddd55555555d11111111111133333333636464649999994444444444d4444444449999555555566611111dd666666b66b4444444
5555554444444333ddddddddddd5555555511111111111113333333336565655999999444444444444444444449999555555566611111dd666666b66b4444444
5555554444444333ddddddddddd555555551111111111111333333333365655599999944444444444444444444999955555556661111111666666b66b4444444
5555555444444333ddddddddddd555555551111111111111333333333356555555555544444444444444444444999955555556661111111666666b66b4444445
666666544444ddddddddd777ddd6666666611111111111113333333333555555555555444444444444444444449999555555ddddddd11116666bb6dddbbd4445
666666544444ddddddddd777ddd6666666611111111111113333333333555555555555444444444444444444449999555555ddddddd1111666b666dddddb4445
666666544444ddddddddd777ddd6666666611111111111111111333333555555555555444444444444444444455555555555ddddddd1111111b666dddddb4445
666666544433ddddddddddddddd6666666611111111111111111333333555555555555444444444444444444455555555555ddddddd111111b6666ddddddb445
666666544433ddddddddddddddd6666666611111111111111111333333555555555555444444444444444d44455555555555ddddddd11111b16666dddddd4b45
666666544433ddddddddddddddd666666661111111111111111133333355555555555544444444444444d4d4455555555555ddddddd1111b116666dddddd44b5
666666544dddddddddddddddddd66666666111111111111111113333335555555555554444444444444d4d4d455555555555ddddddd1111b116666dddddd44b5
666666544dddddddddddddddddd6666666611111111111111111111111555555555555444444444444d4d4d4d55555555555ddddddd1111bbbb11bbbbddbbbb5
666666544dddddddddddddddddd66666666111111111111111111111115555555555555555555555555d5d4d455555555555ddddddd1111111b11bddbddbddd5
666666544dddddddddddddddddd666666661111111111111111133333355555555555555555555555555d4d4455555555555ddddddd11111111bb1dddbbdddd5
666666544dddddddddddddddddd6666666611111111111111111333333555555555555555555555555555d44455555555555ddddddd11111111111ddddddddd5
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0281820566666505666665056666650000000000000000000000000049aaa940049aaa9400000000000000000097a94056666650566666505666665056666650
08e88806600066066000660660006600000000000000000000000000124a92000029a42100000000000000000047a92066000660660006606600066066000660
0288820770007705777777077000770000000000000000000000000004a9400484049a4000000000000000000014941077000770770007707700077077000770
00282006600066000000660660006600000000000000000000000000002a924a9129a20000000000000000000002920066000660660006606600066066000660
00020001ddddd10dddddd101ddddd100000000000000000000000000000149a4249410000000000000000000004999201ddddd101ddddd101ddddd101ddddd10
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
01100000322233e2020e2020e2070e2020e2020e2020e2021f60124401263051f3051f20526305284051f205022000220002200022000e2000e200266040e2003260302200022000220011200112003260411200
010c00001a137260371a117260171a0031a0031a0031a003024020240202402024020240202402024020240202402024020240202402024020240202402024020240202402024020240202402024020240202402
010c00000024500445003450c4451824500445003450c4450024500445003450c4451824500445003450c4450024500445003450c4451824500445003450c4450024500445003450c44518245004450c34500445
010c00000a2450a4450a34516445222450a4450a345164450a2450a4450a34516445222450a4450a345164450a2450a4450a34516445222450a4450a345164450a2450a4450a34516445222450a445163450a445
010c00000824508445083451444520245084450834514445082450844508345144452024508445083451444508245084450834514445202450844508345144450824508445083451444520245084451434508445
010c0000052450544505345114451d245054450534511445052450544505345114451d245054450534511445072450744507345134451f245074450734513445072450744507345134451f245074451334507445
010c00000c2450c4450c34518445242450c4450c345184450c2450c4450c34518445242450c4450c345184450c2450c4450c34518445242450c4450c345184450c2450c4450c34518445242450c445183450c445
010c00000e2370f3370e2170f3171b2071d2071f500295001f50000000205000000000000000001f5000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
010c00200c3350c3351f2250c2353c615306141b335246030c2350c335184350c2353c61524614002350c6140c3350c3350c3050c2353c61524614183350c6140c2350c2351f2250c3353c61524614002350c604
010c00000d3050d305112050d20530603256031b305246030c2050c305184050c205306032460300205032040c3050c305004020c205306032460318305246030c2050c2051f2050c30530603246030020524000
010c001018335274151b335183151f2351b31524435273251f33524415263351f3251f23526315274351f21500000000000000000000000000000000000000000000000000000000000000000000000000000000
011000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
010c00101a335294151c3351a315212351c31526435212151f33526415263351f3152123526315294352121502405024050e4050e40502405024050e4050e40502405024050e4050e40502405024050f4050c405
010c001018335284151c335183151f2351c315244351f2151f33524415263351f3151f23526315284351f2151a200262051b200000001f200000002420000000282000000024200000001f200000001c20000000
010c00080274202745027420274502742027450274202745027020270502702027050270202705027020270502702027050270202705027020270502702027050270202705047020270505702047050970205705
010c00080074200745007420074500742007450074200745007020070200702007020070200702007020070200702007020070200702007020070200702007020070200702007020070200702007020070200702
010c00081a725157251c7251d725217251d7251a7251c7251d000153021a3021d302213021d3021a3021d3021a302023020230202302023020230202302023020230202302023020230202302023020230202302
010c00081872513725187251c7251f7251c725187251c725007000030200300002000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
010c00080a7450a7450a7450a7450a7450a7450a7450a7450a7020a7020a7020a7020a7020a7020a7020a7020a7020a7020a7020a7020a7050a7050a7050a7050a7050a7050a7050a7050a7050a7050a7050a702
010c00080874508745087450874508745087450874508745087050870508705087050870508705087050870508705087050870508705087050870508705087050870508705087050870508705087050870508705
010c00000574505745057450574505745057450574505745057450574505745057450574505745057450574507745077450774507745077450774507745077450774507745077450774507745077450774507745
010c00080c7450c7450c7450c7450c7450c7450c7450c745000020c7020c7020c7000c7000c7000c7000c7000c7000c7000c7000c7000c7000c7000c7000c7000c7000c7000c7000c7000c7000c7000c7000c705
010c00080074500745007450074500745007450074500745000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
010c00080274502745027450274502745027450274502745000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
010c00081d7151a7151c7151a715157151c7151d715217151d7001a7001c700000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
010c00081c715187151c7151871513715187151c7151f7151c7051c70500000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
011000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
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
01 10080e1c
00 10090e18
00 100a0e19
00 100b0e1a
00 100a0e19
00 10090e18
00 100c0e1b
02 10080e1c
01 1214161e
00 1214161e
00 1315171f
02 1315171f
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

