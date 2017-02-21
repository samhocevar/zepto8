pico-8 cartridge // http://www.pico-8.com
version 8
__lua__
-- zepton
-- by rez

-- todo
-- Öspaceship sprites
-- Öblink red latest score

function _init()
	cls()
	cartdata"zepton"
	snd=dget"8"<1 and true or false
	dbg=dget"9">0 and true or false
	mode=1  --o=game/1=menu/2=score
	fps={}
	for i=0,23 do fps[i]=0 end
	f=128   --focale
	d=512   --depth
	nx=40   --size x
	nz=48   --size z
	s=0     --speed
	u=8     --unit
	u2,u3,u4,uh=2*u,3*u,4*u,u/2
	uz=d/nz --depth unit
	shp={}  --spaceship
	su=1    --èunit
	su2,suh=2*su,su/2
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
	init()
	if(snd) music(0)
	menuitem(1,"view score",
		function()
			mode=2
			init()
		end)
	menuitem(2,"reset score",
		function()
			for i=0,7 do
				scr[i]=0
				dset(i,0)
			end
		end)
	menuitem(3,"sound on/off",mu)
	menuitem(4,"debug mode",db)
end

function init()
	t=0     --timer
	fc=0    --frame count
	tv={}   --terrain
	tz={}   --èdistance
	tn=0    --ècounter
	px,py,pz=0,0,0
	se=100  --energy level
	sx=0
	sy=8*u
	sz=3.5*uz
	sh=0    --shield
	ix=0
	iy=0
	mx=0    --mire x
	my=0    --mire y
	mz=0    --mire z
	mt=false--ètarget
	mf=0    --èflag
	msl={}  --missile
	mn=0    --ènumber
	mp=0    --èposition
	bul={}  --bullet
	bn=0    --ènumber
	bp=0    --èposition
	foe={}  --enemy
	er=12   --èrate
	lsr={}  --laser
	lr=256  --èrate
	smo={}  --smoke
	exp={}  --explosion
	sta={}  --stars
	sc=0    --score
	cs=0    --camera shake
	if mode==0 then
		sn=32
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
			a=v.a+0.25+(sx+sy)/720
			x=v.r*cos(a)+128-sx
			y=v.r*sin(a)-sy
			z=(v.z+256)%256
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
	local y,w,c,bt,by,bw,bh,bc
	local p,vy,bx,ry
	local tc={7,10,9,4,5,3,11,12}
	for j=0,nx-1 do
		----------------------terrain
		y=u2*cos(4/nz*min(a%nz,nz/4))
		y+=u2*cos(3/nx*j) --x
		y+=u2*sin((a-j*4)/nz) --y
		y+=u3*sin((2/nz)*(a+j))
		y=flr(min(u2+y,u4)/uh)*uh
		w=1
		c=tc[flr(max((u4+y)/8,1))]
		bt=0
		by=u4
		bw=1
		bh=u
		bc=0
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
			c=12
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
				bw=0.5
				by=ry-uh
				bc=7
				bt=1
				bw=0.25
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
				bc=8
			end
			if y<ry then --wall
				c=a%4<2 and 6 or 13
			end
		end
		l[j]={
			x=(j-nx/2)*u,
			y=y,
			w=w,
			c=c,
			by=by,
			bw=bw,
			bh=bh,
			bc=bc,
			bt=bt,
			e=false}
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
				v.c=12
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
			l[k].w,l[k].c=1,12
		end
		return 1
	end
	return p
end

function aam()
	local x,v
	x=mn%2==0 and su or -su
	v={
		x=-sx+x,y=-sy,z=sz,
		vx=x/2,vy=suh,vz=0,
		rx=1,ry=1}
	if mt then
		v.t=mt
	else
		v.t={
			x=mx,y=my,z=d,
			w=0,h=0,l=false}
	end
	add(msl,v)
	mn+=1
	if(snd) sfx(2,-1)
end

function fire()
	local x,v
	x=bn%2==0 and su2 or -su2
	v={x=-sx+x,y=-sy,z=sz}
	add(bul,v)
	bn+=1
	if(snd) sfx(1,-1)
end

function ptr(x,y,r)
	--pset(x-r,y)
	pset(x-r+1,y-1)
	pset(x-r+1,y+1)
	--pset(x+r,y)
	pset(x+r-1,y-1)
	pset(x+r-1,y+1)
	--pset(x,y-r)
	pset(x-1,y-r+1)
	pset(x+1,y-r+1)
	--pset(x,y+r)
	pset(x-1,y+r-1)
	pset(x+1,y+r-1)
	for i=0,r/2,3 do
		pset(x-r+i,y)
		pset(x+r-i,y)
		pset(x,y-r+i)
		pset(x,y+r-i)
	end
end

function lock(x,y,w,h,c)
	pal(8,c)
	spr(12,x-w-1,y-h-1)
	spr(13,x+w-6,y-h-1)
	spr(28,x-w-1,y+h-6)
	spr(29,x+w-6,y+h-6)
	pal()
end

function spawn(i,j)
	local y,w,h,v
	y=-u4-rnd(16)*u
	w=1+flr(rnd(9))
	h=1+flr(rnd(9))
	v={
		t=t,
		e=(w+h)*5,    --energy
		f=(w+h)*5,
		k=false,
		x=(-nx/2+j)*u+uh,
		y=y,
		z=tz[i],
		w=w*su,
		h=h*su,
		py=y,
		vy=su/4,
		ry=1.03125,
		bl=8+rnd(16), --bolt length
		bt=rnd(232),  --bolt time
		a=rnd(360),
		v=tv[i][j]}
	add(foe,v)
	tv[i][j].e=v
end

function enemy(e)
	local zf,zu,x,y,w,h,g
	zf=1/e.z*f
	zu=zf*su
	x=(px+e.x)*zf
	y=(py+e.y)*zf
	w=e.w*zf-zu-1
	h=e.h*zf-zu-1
	g=(py+e.v.y)*zf
	color(0)
	rectfill(x+w,y+zu,x-w,y-zu)
	rectfill(x+zu,y+h,x-zu,y-h)
	circfill(x,y,2*zu)
	circfill(x-w,y,zu)
	circfill(x+w,y,zu)
	circfill(x,y-h,zu)
	circfill(x,y+h,zu)
	if e.e>0 then
		color(8)
		circfill(x,y,zu)
		if e.w>3*su and zu>1 then
			circfill(x-w+su,y,zu/2)
			circfill(x+w-su,y,zu/2)
		end
	end
	--line(x,y+h,x,g)
	if e.e>0 -----------------bolt
	and e.y>-16*u and e.z>sz
	and t%256>=e.bt
	and t%256<e.bt+e.bl then
		local n,x1,y1,x2,y2
		n=16
		x1=x
		y1=y+h+zu*2
		color(7)
		circfill(x1,y1,zu/2)
		for i=0,n-2 do
			x2=x1+rnd(8*zu)-4*zu
			y2=y1+((e.v.y-e.y)/n*zf)
			line(x1,y1,x2,y2)
			x1=x2
			y1=y2
		end
		if t%256>e.bt+e.bl-1 then
			e.bl=4+rnd(28)
			e.bt=rnd(256-e.bl)
		end
	end
end

function laser(x,y,z,a,b)
	add(lsr,{
		x=x,y=y,z=z,
		a=a,b=b,r=0})
	if(snd) sfx(4,-1)
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
	if(snd) sfx(3,-1,8/d*z)
	cs=2/d*(d-sz-z)
end

function smoke(x,y,z,w)
	add(smo,{
		i=0,
		x=x,y=y,z=z,w=w,
		vx=(rnd(12)-6)/256,
		vy=(rnd(12)-6)/256,
		vz=0})
end

function ct(i,j,y,z)
	local v,zi=tv[i][j],tz[i]
	if z>zi then
		if(y>v.y) return 1
		if(y>v.by and y<v.by+v.bh) return 2
	end
	return 0
end

function dead()
	se=0
	s=0.5
	ex(-sx,-sy,sz)
	if(snd) sfx(-1,0)
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
	local cx,cy,c,n,a,b
	local x,y,z,zf,zu,w,h,r,x2,y2
	px=sx*0.9375
	py=sy*0.9375+u2
	a=t/10
	cx=-64+sx/4--+cs*cos(a)
	cy=-48+sy/4+cs*sin(a)
	ch=cy+120
	camera(cx,cy)
	y=u2/d*f
	w=cx+127
	---------------------------sky
	color(1)
	rectfill(w,y-21,cx,cy)
	color(5)
	line(cx,y-22,w,y-22)
	rectfill(w,y-10,cx,y-20)
	color(13)
	line(cx,y-12,w,y-12)
	rectfill(w,y-5,cx,y-10)
	color(14)
	line(cx,y-6,w,y-6)
	rectfill(w,y-3,cx,y-4)
	color(15)
	line(w,y-2,cx,y-2)
	color(7)
	line(w,y-1,cx,y-1)
	color(6)
	line(cx,y,w,y)
	-------------------------stars
	c={7,6,13,13}
	for i=0,sn-1 do
		v=sta[i]
		x=(v.x-cx-sx/4)%128
		if(i%16) color(c[flr(i/8)+1])
		pset(cx+x,v.y-56+y)
	end
	x=-sx/6-8
	spr(14,x,y-30,2,2) --moon
	spr(47,x+16,y-38)  --sun
	spr(46,x+48,y-32)  --venus
	spr(62,x-48,y-15)  --titan
	spr(63,x-32,y-32)  --neptune
	---------------------------sea
	color(12)
	rectfill(w,ch,cx,y+1)
	-------------------------voxel
	for i=pz-1,0,-1 do dt(i) end
	for i=nz-1,pz,-1 do dt(i) end
	---------------------explosion
	c={7,10,9,8,4,5,13,6}
	for v in all(exp) do
		zf=1/v.z*f
		x=(px+v.x)*zf
		y=(py+v.y)*zf
		w=v.w*zf
		color(c[min(flr(12/v.w),7)])
		circfill(x,y,w)
	end
	------------------------bullet
	color(9)
	for b in all(bul) do
		for i=0,3 do
			zf=1/(b.z+(2-i)*su)*f
			x=(px+b.x)*zf
			y=(py+b.y)*zf
			w=su/8*zf
			rectfill(x+w,y+w,x-w,y-w)
		end
	end
	-----------------------missile
	c={8,8,8,5,9}
	for m in all(msl) do
		for i=0,3 do
			zf=1/(m.z+(2-i)*su)*f
			x=(px+m.x)*zf
			y=(py+m.y)*zf
			w=su/4*zf
			color(c[i+1])
			rectfill(x+w,y+w,x-w,y-w)
		end
		if m.vz>suh then
			zf=1/(m.z-su2)*f
			x=(px+m.x)*zf
			y=(py+m.y)*zf
			color(10)
			circfill(x,y,su*zf)
		end
	end
	------------------------smoke1
	for v in all(smo) do
		if v.z>sz then
			zf=1/v.z*f
			x=(px+v.x)*zf
			y=(py+v.y)*zf
			w=v.w*su*zf
			i=64+flr(rnd(2))*16
			spr(i+min(w,7),x-3,y-3)
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
			color(v.c)
			if v.c!=6 then
				w=v.w*zf-1
				h=v.h*zf-1
				rectfill(x+w,y+h,x,y)
			else
				circfill(x+zu,y+zu,zu)
			end
			--w=zu-4
			--if btn(5) and fc%2==0 then
			--	spr(36+flr(rnd(4)),x+w,y+w)
			--end
			--w=zu-4
			--spr(s<0.5 and 35 or 34,x+w,y+w)
		end
		if sh>0 then ----------shield
			zf=1/sz*f
			x=(px-sx)*zf
			y=(py-sy)*zf
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
			zf=1/v.z*f
			x=(px+v.x)*zf
			y=(py+v.y)*zf
			w=v.w*su*zf
			i=64+flr(rnd(2))*16
			spr(i+min(w,7),x-3,y-3)
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
			w=mt.w*zf
			h=mt.h*zf
			if(mt.l) c=11
			lock(x,y,w,h,c)
			color(c)
			print(flr(mt.z-sz),x+w+3,y-2)
			print(-flr(-mt.e/50),x-w-5,y-2)
			v=(w*2+2)/mt.f*mt.e
			line(x-w-1,y+h+3,x-w-1+v,y+h+3)
			if mt.l and mf!=1 then
				if(snd) sfx(0,-1)
				mf=1
			end
		else
			mf=0
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
	x=(px+mx)*zf
	y=(py+my)*zf
	r=1
	if mz==d then
		color(8)
	else
		if(mt) r=(mt.w+mt.h)*zf
		if se>0 then
			zf=1/(sz+u)*f
			x2=(px-sx)*zf
			y2=(py-sy)*zf
			n=(x2-x)/(y2-y)
			for i=0,y2-y-1,3 do
				pset(x+n*i,y+i)
			end
		end
	end
	ptr(x,y,r)
	if btn(5) then
		circ(x,y,r)
	end
	---------------------------hud
	camera(0,0)
	h=24
	n=h/4
	color(0)
	rectfill(8,h,4,1)
	color(7)
	for i=0,3 do
		y=1+(n*i+py+n/2)%h
		pset(5,y)
		y=1+(n*i+py)%h
		line(5,y,6,y)
	end
	y=1+h/2
	color(8)
	line(5,y,6,y)
	color(0)
	print(flr(sy+u4),10,y-2)
	--print(flr(-sx),10,y-8)
	-------------------------speed
	y=(h-1)/2*s
	rectfill(1,1,2,h)
	if y>0 then
		color(2)
		rectfill(1,h-y,2,h)
	end
	--color(8)
	--pset(1,h-y)
	--pset(2,h-y)
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
	end
	---------------------------bar
	camera(0,-121)
	color(0)
	rectfill(127,6,0,0)
	spr(48,56,0,2,1) --eagle
	spr(50,-1)
	nbr(se,7,1,3,100)
	spr(51,88)
	nbr(sc,96,1,4,1000)
	-------------------------debug
	if dbg then
		color(5)
		spr(52,28)
		print(count(msl),35,1)
		spr(53,36)
		print(count(bul),43,1)
		spr(65,72)
		print(count(smo),78,1)
		--spr(100,71,-1)
		--print(count(lsr),78,1)
	end
end

function dt(i)
	local zi,zf,zu,x,y,w,h,e,v
	zi=tz[i]
	zf=1/zi*f
	zu=zf*u
	e=false
	for j=0,nx-1 do
		v=tv[i][j]
		if v.c!=12 then
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
end

function upd()
	local x,y,z,w,v,a,b,c
	if(sh>0) sh-=1
	if(cs>0) cs-=0.0625
	if(abs(cs)<0.0625) cs=0
	if t%512==0 then -----level up
		if(er>2) er-=1
		if(er<3 and lr>64) lr=flr(lr/2)
	end
	s=2/248*(24+sy+u4)
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
		w=su/4
		y=-sy-suh
		z=sz-su-rnd(0.75)*uz
		b=not btn(4) and 1 or 4
		if fc%b==0 then
			if flr(rnd(2))==0 then
				smoke(-sx-su,y+ix/4,z,w)
			else
				smoke(-sx+su,y-ix/4,z,w)
			end
		end
	end
	if fc%2==0 then
		for m in all(msl) do
			if m.vz>suh then
				smoke(m.x,m.y,m.z-su2,su)
			end
		end
	end
	for v in all(smo) do
		v.i+=1
		v.x+=v.vx
		v.y+=v.vy
		v.z-=s--+v.vz
		if v.i<16 then
			v.w+=su/32
		else
			v.w-=su/64
		end
		--if(v.vz>s/4) v.vz-=s/16
		if(v.z<1 or v.w<0) del(smo,v)
	end
	-------------------------enemy
	for e in all(foe) do
		e.z-=s
		if(e.z<1) del(foe,e)
		if e.e<1 then
			if(mt==e) mt=false
			if fc%2==0 then
				x=e.x+rnd(e.w*2)-e.w
				smoke(x,e.y,e.z,su)
			end
			e.y+=e.vy
			e.vy*=e.ry
			if e.y>e.v.y then --shutdown
				e.k=true
				v=e.v
				v.by=v.y
				v.bc=0
				v.bt=0
				v.e=false --no reference?
				del(foe,e)
				ex(e.x,v.y,e.z)
			end
		end
		if se>0 and e.z>sz+su2
		and (t-e.t)%lr==0 then
			z=e.z-sz-4*uz*s
			b=0.25+atan2(e.x+sx,z)
			c=0.25+atan2(e.y+sy,z)
			laser(e.x,e.y,e.z,b,c)
		end
	end
	-------------------------laser
	for l in all(lsr) do
		l.z-=s
		l.r+=4
		x=l.x+l.r*sin(l.a)
		y=l.y+l.r*sin(l.b)
		z=l.z-l.r*cos(l.a)
		if se>0 and z<sz+su2 then
			if abs(-sx-x)<su2
			and abs(-sy-y)<su2 then
				del(lsr,l)
				se-=5
				s=0.125
				sh=8
				cs=1.5
				if(snd) sfx(5,-1)
				if(se<1) dead()
			end
		end
		if(abs(x)>nx/2*u or z<0) del(lsr,l)
	end
	---------------------spaceship
	for e in all(foe) do
		if abs(e.z-sz)<su2 then
			if (abs(e.x+sx)<e.w+su2 and
			abs(e.y+sy)<su) or
			(abs(e.y+sy)<e.h+su and
			abs(e.x+sx)<su2) then
				e.e=0
				if(se>0) dead()
				break
			end
		end
	end
	--------------------------mire
	mx=-sx
	my=-sy
	mz=d
	if(mt and mt.z<sz+su2) mt=false
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
				break
			end
		end
		if mz==d then
			for i=0,pz-1 do
				v=tv[i][j]
				if -sy>v.y or (-sy>v.by
				and -sy<v.by+v.bh) then
					mz=tz[i]
					break
				end
			end
		end
		if mz<d then
			mt={
				e=0,
				x=mx,y=my,z=mz,
				w=uh,h=uh,
				l=true}
		end
		if(se>0 and sz+su2>mz) dead()
	end
	------------------------bullet
	for b in all(bul) do
		b.z+=8-s
		if b.y<-u4 then --air to air
			for e in all(foe) do
				if b.z>e.z then
					if (abs(e.x-b.x)<e.w and
					abs(e.y-b.y)<su) or
					(abs(e.y-b.y)<e.h and
					abs(e.x-b.x)<su) then
						del(bul,b)
						boom(b.x,b.y,e.z,uh)
						--smoke(b.x,b.y,e.z,su2)
						if e.e>0 then
							e.e-=5
							if e.e<1 then
								sc+=flr(e.f/10)
							else
								sc+=1
							end
						end
					end
				end
			end
		else ----------air to surface
			local j,ctv,cti,v2=0,0,0
			j=flr((b.x+nx/2*u)/u)
			for i=pz,nz-1 do
				ctv=ct(i,j,b.y,b.z)
				if ctv!=0 then
					cti=i
					break
				end
			end
			if ctv==0 then
				for i=0,pz-1 do
					ctv=ct(i,j,b.y,b.z)
					if ctv!=0 then
						cti=i
						break
					end
				end
			end
			if ctv!=0 then
				del(bul,b)
				smoke(b.x,b.y,b.z,su2)
			end
		end
		if(b.z>d) del(bul,b)
	end
	-----------------------missile
	for m in all(msl) do
		v,x,y=m.t,0,0
		if v.l then
			x=(v.x-m.x)/16
			y=(v.y-m.y)/16
		end
		m.x+=m.vx+x*m.vz
		m.y+=m.vy+y*m.vz
		m.z+=m.vz
		m.vx*=m.rx
		m.vy*=m.ry
		if(m.rx>0) m.rx-=0.0078125
		if(m.ry>0) m.ry-=0.0078125
		m.vz+=0.03125 --1/32
		if(m.t.y>-u4) m.t.z-=s
		if(m.z>d) del(msl,m)
		if not v.l and m.z>d/2 then
			del(msl,m)
			ex(m.x,m.y,m.z)
		end
		if m.y<-u4 then --air to air
			if m.z>v.z then
				if (abs(v.x-m.x)<v.w and
				abs(v.y-m.y)<su) or
				(abs(v.y-m.y)<v.h and
				abs(v.x-m.x)<su) then
					del(msl,m)
					ex(v.x,v.y,v.z)
					if v.e>0 then
						v.e-=50
						if(v.e<1) sc+=flr(v.f/5)
					end
				end
			end
		else ----------air to surface
			local j,ctv,cti,v2=0,0,0
			j=flr((m.x+nx/2*u)/u)
			for i=pz,nz-1 do
				ctv=ct(i,j,m.y,m.z)
				if ctv!=0 then
					cti=i
					break
				end
			end
			if ctv==0 then
				for i=0,pz-1 do
					ctv=ct(i,j,m.y,m.z)
					if ctv!=0 then
						cti=i
						break
					end
				end
			end
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
		if v.z<1 or v.w<0 then
			del(exp,v)
		end
	end
end

function menu()
	local cx,cy,n,c,c1,c2,y,k
	cx,cy=-64,-64
	camera(cx,cy)
	color(0)
	rectfill(cx+127,cy+127,cx,cy)
	a=t/1080
	sf()
	if mode==1 then ----------logo
		camera(-16,-56)
		n=flr(t/4)%128
		for j=0,14 do
			c1=sget(n+j,64)
			c2=sget(n-j,65)
			for i=0,92 do
				c=sget(i,80+j)
				if(c==3 and c1!=0) pset(i,j,c1)
				if(c==11 and c2!=0) pset(i,j,c2)
			end
		k="éstart"
		end
	else --------------------score
		for i=0,7 do tri(i) end
		camera(-44,-34)
		spr(107,0,0,5,1)
		for i=0,7 do
			y=12+i*6
			spr(122,-8,y)
			nbr(i+1,0,y,1,1)
			spr(123,8,y)
			nbr(scr[i],16,y,4,1000)
		end
		k="éexit"
	end
	camera(0)
	color(1)
	print(k,1,122)
	color(snd and 1 or 8)
	print("ç",121,122)
end

function sf() --------starfield
	local v,x,y,z,b,zf,c,k,n
	c={1,2,14,15,7,8,
				1,5,13,12,7,8}
	for i=0,sn-1 do
		v=sta[i]
		b=v.a+cos(a)/4+(sx+sy)/720
		x=v.r*cos(b)+128*cos(a*2)-sx
		y=v.r*sin(b)+128*sin(a/2)-sy
		z=(v.z+256*sin(a))%256
		if(z<0) z+=256
		zf=1/z*32
		x,y=x*zf,y*zf
		v.t[0]={x=x,y=y,z=z}
		for j=5,1,-1 do
			v.t[j]=v.t[j-1]
		end
		if abs(v.t[0].z-v.t[5].z)<128
		and abs(x)<80 and abs(y)<80 then
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
		x=v.x+128*cos(a*2)-sx
		y=v.y+128*sin(a/2)-sy
		z=(256-i*32+256*sin(a))%256
		if(z<0) z+=256
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

function nbr(n,x,y,l,k)
	for i=0,l-1 do
		spr(112+((n/k)%10),x+i*8,y)
		k/=10
	end
end

function mu()
	snd=not snd
	dset(8,snd and 0 or 1)
	if snd then
		if(mode!=0) music(0)
	else
		music(-1)
		for i=0,3 do sfx(-1,i) end
	end
end

function db()
	dbg=not dbg
	dset(9,dbg and 1 or 0)
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
	t+=1--time()*32
	if(mode==0) upd()
	-------------------------input
	local xm,yt,im,ii,id,iz
	xm=18*u     --x max
	yt=24*u     --y top
	im=2
	ii=0.125
	id=0.0625   --1/16
	if se>0 and s<1 then
		s+=0.015625 --1/64
		--if(snd) sfx(6,0)
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
	if btnp(4) then
		if mode==0 and se<1 then
			mode=2
			init()
			if snd then
				music(0)
				sfx(-1,0)
			end
			return
		end
		if mode==1 then
			mode=0
			init()
			if snd then
				music(-1)
				--sfx(6,0)
			end
			return
		end
		if mode==2 then
			mode=1
			return
		end
	end
	if(btnp(5,1)) mu() ------sound
	if(btnp(4,1)) db() ------debug
	if(btn(0,1) and dbg) s=0
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
55500000333000005555555511111111000000000000000000000000000000000000000000000000000000000000000008800000000008800000001212200000
5d5ddd00373222005dddddd512222221000dd0000001100000000000000000000000000000000000000000000000000080000000000000080000d11111528000
555555d0333222205d6666d51233332100555500002222000000000000000000000000000000000000000000000000008000000000000008000651111111d900
0d55755d022222225d6dd6d5123443210d5dd5d0012772100000000000000000000000000000000000000000000000000000000000000000006d111111111d80
0d57775d022222225d6dd6d5123443210d5dd5d00127721000000000000000000000000000000000000000000000000000000000000000000d65111111111120
0d55755d022222225d6666d512333321005555000022220000000000000000000000000000000000000000000000000000000000000000000661111111111152
00d555d0002222205dddddd512222221000dd0000001100000000000000000000000000000000000000000000000000000000000000000005661111111111112
000ddd0000022200555555551111111100000000000000000000000000000000000000000000000000000000000000000000000000000000d665111111111111
0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000677d111111111112
0b000000060000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000005666111111111111
0000000000000000000000000000000000dddd000077770000000000000000000000000000000000000000000000000000000000000000000777d11111111110
0000000000000000000000000000000000d11d000078870000000000000000000000000000000000000000000000000000000000000000000d777d11111115d0
0000000000000000000000000000000000d11d0000788700000000000000000000000000000000000000000000000000000000000000000000c7776d5115d600
0000000000000000000000000000000000dddd00007777000000000000000000000000000000000000000000000000008000000000000008000c777777776000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000080000000000000080000d677776d0000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000008800000000008800000005dd5000000
00000000000000000000000000000000000090000000090090000009009000000000000000000000000000000000000000000000000000000000000000000000
000200000000000000555500005555000009a00000009a000a9009a000a900000000000000000000000000000000000000000000000000000012210000012100
060206000001000005499450055115500009a0009a99a90009a99a90009a99a900000000000000000000000000000000000000000000000001dccd1000149410
06d2d60000010000059aa950051111509aaaa99009aaa900009aa900009aaa9000000000000000000000000000000000000000000000000002cccc200029a920
02d2d20000000000059aa95005111150099aaaa9009aaa90009aa90009aaa90000000000000000000000000000000000000000000000000002cccc2000149410
22525220e00e00e00549945005511550000a9000009a99a909a99a909a99a90000000000000000000000000000000000000000000000000001dccd1000012100
20000020e00e00e00055550000555500000a900000a900000a9009a000009a000000000000000000000000000000000000000000000000000012210000000000
00000000000000000000000000000000000900000090000090000009000009000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000012100000
49aaa940049aaa940028182000577650000181000000000000000000000000000000000000000000000000000000000000000000000000000055550028200000
124a92000029a421008e888000766d6000028200000282000000000000000000000000000000000000000000000000000000000000000000053bb35012100000
04a9400484049a40002888200070d060000282000008a80000000000000000000000000000000000000000000000000000000000000000000dbbbbd000000000
002a924a9129a20000028200005706500008880000028200000000000000000000000000000000000000000000000000000000000000000005bbbb5000000000
000149a42494100000002000000676000002a2000000000000000000000000000000000000000000000000000000000000000000000000000d3bb3d0000002d2
000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000dddd0000000d6d
000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000002d2
00000000000000000000000000000000000000000006000000060600060606000000000000000000000000000000000000000000888880000008888800088880
00000000000000000000000000060000000060000060600000606060606060600008000000000000000000000000000000000000888888000088888800888888
00000000000f00000000f00000f0f000000606000606060006060600060606000000000000000000000000000000000000000000880088800888000008880088
000f0000006060000006060006060600006060606060606060606060606060600809080000000000000000000000000000000000880008888880088888800088
00000000000f000000f0f0000060f000060606000606060006060600060606000000000000000000000000000000000000000000880000888800888888888888
0000000000000000000600000006000000f060000060600060606000606060600008000000000000000000000000000000000000880000888800000088888888
00000000000000000000000000000000000600000006000006060000060606000000000000000000000000000000000000000000888888888888888888000088
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000888888800888888888000088
00000000000000000000000000000000000000000006000006060000060606000000000000000000000000000000000011101110111011101110101011101110
00000000000000000000000000060000000600000060600060606000606060600002000000000000000000000000000010001010101000001010101000000010
0000000000060000000f000000f06000006060000606060006060600060606000000000000000000000000000000000010101110101011101010101011101100
0006000000f0f0000060600006060f00060f0600606060606060606060606060020e020000000000000000000000000010101010101010001010101010001010
0000000000060000000f0f0000f06000006060600606060006060600060606000000000000000000000000000000000011101010101011101110011011101010
00000000000000000000600000060000000606000060600000606060606060600002000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000060000006000000060600060606000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000002222000000000000000000000000005ddddd505ddddd505ddddd505ddddd505ddddd50
0000000000000000000000000000000000000000000220000028820002888820000100000000000000000000ee200000ee000000ee000ee0ee000ee0ee000000
00000000000000000002200000022000002882000088880002899820289aa9820000000000000000000000002efffe20ff200000ff200ff0ff20ef20ff20e000
000200000002200000288200002992000089980002899820089aa98028a77a82010d0100000000000000000000002ee0eee00000eee00ee0eee02ee0eee00000
000000000002200000288200002992000089980002899820089aa98028a77a820000000000000000000000009aaaa94049aaaa9049aaa9409aa00a9049aaaa90
00000000000000000002200000022000002882000088880002899820289aa9820001000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000002200000288200028888200000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000002222000000000000000000000000000000000000000000000000000000000000000000
56666650666650006666665066666650660006606666666056666660666666605666665056666650000056606650000000000000000000000000000000000000
66000660005660000000066000000660665006606600000066000000000006606600066066000660000066000660000000000000000000000000000000000000
77000770000770005777775000077750577777707777775077777750005777505777775057777770000077000770000000000000000000000000000000000000
66000660000660006600000000000660000006600000066066000660006650006600066000000660000066000660000000000000000000000000000000000000
1ddddd10ddddddd0ddddddd0dddddd1000000dd0dddddd101ddddd1000dd00001ddddd10dddddd1000001dd0dd10000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0000000000000000125def7a9845215d676d51000000000000000000113b7b3110000000000000000000000012489999999999a77a9999999999842100000000
0000000015d67d12489ae21000000000000000000000113b7b311000000000000000000000000000000000000000000000000000000000000000000000000000
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
33333333333333330033333333333333003333333333300333333333333333333003333333333000330000000000300000000000000000000000000000000000
03bbbbbbbbbbbb3003bbbbbbbbbbbb3003bbbbbbbbbbb3003bbbbbbbbbbbbbb3003bbbbbbbbbb3003b3000000003300000000000000000000000000000000000
003bbbbbbbbbb30003bbbbbbbbbbb30003bbbbbbbbbbbb3003bbbbbbbbbbbb3003bbbbbbbbbbbb303bb30000003b300000000000000000000000000000000000
000333333bbb300003bb33333333300003bb33333333bb300033333bb333330003bbb333333bbb303bbb300003bb300000000000000000000000000000000000
00000003bbb3000003bb33333300000003bb3333333bbb300000003bb300000003bb30000003bb303bbbb30003bb300000000000000000000000000000000000
0000003bbb30000003bbbbbb3000000003bbbbbbbbbbb3000000003bb300000003bb30000003bb303bb3bb3003bb300000000000000000000000000000000000
000003bbb300000003bbbbb30000000003bbbbbbbbbb30000000003bb300000003bb30000003bb303bb33bb303bb300000000000000000000000000000000000
00003bbb3000000003bb33300000000003bb3333333300000000003bb300000003bb30000003bb303bb303bb33bb300000000000000000000000000000000000
0003bbb33333330003bb33333333300003bb3000000000000000003bb300000003bbb333333bbb303bb3003bb3bb300000000000000000000000000000000000
003bbbbbbbbbbb3003bbbbbbbbbbb30003bb3000000000000000003bb300000003bbbbbbbbbbbb303bb30003bbbb300000000000000000000000000000000000
03bbbbbbbbbbbbb3003bbbbbbbbbbb3003bb3000000000000000003bb3000000003bbbbbbbbbb3003bb300003bbb300000000000000000000000000000000000
33333333333333333003333333333333003b3000000000000000003b3000000000033333333330003b30000003bb300000000000000000000000000000000000
0000000000000000000000000000000000033000000000000000003300000000000000000000000033000000003b300000000000000000000000000000000000
00000000000000000000000000000000000030000000000000000030000000000000000000000000300000000003300000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000300000000000000000000000000000000000
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
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
__label__
88888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888
888888888888888888888888888888888888888888888888888888888888888888888888888888888882282288882288228882228228888888ff888888228888
888882888888888ff8ff8ff88888888888888888888888888888888888888888888888888888888888228882288822222288822282288888ff8f888888222888
88888288828888888888888888888888888888888888888888888888888888888888888888888888882288822888282282888222888888ff888f888888288888
888882888282888ff8ff8ff888888888888888888888888888888888888888888888888888888888882288822888222222888888222888ff888f888822288888
8888828282828888888888888888888888888888888888888888888888888888888888888888888888228882288882222888822822288888ff8f888222288888
888882828282888ff8ff8ff8888888888888888888888888888888888888888888888888888888888882282288888288288882282228888888ff888222888888
88888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888
55555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555
55555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555
55555555555555555555555555555555555555555555555500000000000055555555555555555555555555555555555555500000000000055000000000000555
555555e555577757775555e555555555555665666566555506600600000055555555555555555555565555665566566655506660666000055066606660000555
55555ee555575755575555ee55555555556555656565655500600600000055555555555555555555565556565656565655506060606000055060606060000555
5555eee555575757775555eee5555555556665666565655500600666000055555555555555555555565556565656566655506060606000055060606660000555
55555ee555575757555555ee55555555555565655565655500600606000055555555555555555555565556565656565555506060606000055060606060000555
555555e555577757775555e555555555556655655566655506660666000055555555555555555555566656655665565555506660666000055066606660000555
55555555555555555555555555555555555555555555555500000000000055555555555555555555555555555555555555500000000000055000000000000555
55555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555
55555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555
5555555555555555557777756666656666656666655555555555666666665666666665666666665666666665666666665666666665eeeeeeee56666666655555
5555566556656665557557756555656555656565655555555555666776665666667665666666775667777765666677765667666665ee7eee7e56667766655555
55556565655556555577577566656566656565656555555555556676676656667767656666776756676667656666767656767666657e7e7e7e56677776655555
55556565655556555577577565556566556565556555555555556766667656776667656677666756676667656666767657666767657777777756776677655555
5555656565555655557757756566656665656665655555555555766666675766666675776666675777666775777776775766677675e7e7e7e757766667755555
5555665556655655557555756555656555656665655555555555666666665666666665666666665666666665666666665666666665e7eeeee756666666655555
55555555555555555577777566666566666566666555555555555555555555555555555555555555555555555555555555555555555555555555555555555555
55555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555
55555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555
555555555555555555005005005005005dd500566555555555555555555555555555555555555555555555555555555555555555555555555555555555555555
555565655665655555005005005005005dd56656655555555555777777775dddddddd5dddddddd5dddddddd5dddddddd5dddddddd5dddddddd5dddddddd55555
5555656565656555550050050050050057756656655555555555777777775d55ddddd5dd5dd5dd5ddd55ddd5ddddd5dd5dd5ddddd5dddddddd5dddddddd55555
5555656565656555550050050050056657756656655555555555777777775d555dddd5d55d55dd5dddddddd5dddd55dd5dd55dddd55d5d5d5d5d55dd55d55555
5555666565656555550050050056656657756656655555555555777557775dddd555d5dd55d55d5d5d55d5d5ddd555dd5dd555ddd55d5d5d5d5d55dd55d55555
5555565566556665550050056656656657756656655555555555777777775ddddd55d5dd5dd5dd5d5d55d5d5dd5555dd5dd5555dd5dddddddd5dddddddd55555
5555555555555555550056656656656657756656655555555555777777775dddddddd5dddddddd5dddddddd5dddddddd5dddddddd5dddddddd5dddddddd55555
55555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555
55555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555
55555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555
55555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555
55555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555
55500000000000000000000000000000550000000000000000000000000000055000000000000000000000000000005500000000000000000000000000000555
55500000000000000000000000000000550000000000000000000000000000055000000000000000000000000000005500000000000000000000000000000555
55500000000000000000000000000000550000000000000000000000000000055000000000000000000000000000005500000000000000000000000000000555
55500000000000000000000000000000550000000000000000000000000000055000000000000000000000000000005500000000000000000000000000000555
55500000000000000000000000000000550000000000000000000000000000055000000000000000000000000000005500000000000000000000000000000555
55500000000000000000000000000000550000000000000000000000000000055000000000000000000000000000005500000000000000000000000000000555
55500200020002000020000200002000550020002000200002000020000200055002000200020000200002000020005500200020002000020000200002000555
55500000000000000000000000000000550000000000000000000000000000055000000000000000000000000000005500000000000000000000000000000555
55500000000000000000000000000000550000000000000000000000000000055000000000000000000000000000005500000000000000000000000000000555
55500000000000000000000000000000550000000000000000000000000000055000000000000000000000000000005100000000000000000000000000000555
55500770000066000e0000ccc00d0d00550000000000000000000000000000055000000000000000000000000000001710000000000000000000000000000555
55507000000006000e000000c00d0d00550000000000000000000000000000055000000000000000000000000000001771000000000000000000000000000555
55507000000006000eee000cc00ddd00550000000000000000000000000000055000000000000000000000000000001777100000000000000000000000000555
55507000000006000e0e0000c0000d00550000000000000000000000000000055000000000000000000000000000001777710000000000000000000000000555
55500770000066600eee00ccc0000d00550020002000200002000020000200055002000200020000200002000020001771100020002000020000200002000555
55500000000000000000000000000000550000000000000000000000000000055000000000000000000000000000005117100000000000000000000000000555
55500000000000000000000000000000550000000000000000000000000000055000000000000000000000000000005500000000000000000000000000000555
55500000000000000000000000000000550000000000000000000000000000055000000000000000000000000000005500000000000000000000000000000555
55500770000066000e0000ccc00dd000550000000000000000000000000000055000000000000000000000000000005500000000000000000000000000000555
55507000000006000e000000c000d000550000000000000000000000000000055000000000000000000000000000005500000000000000000000000000000555
55507000000006000eee000cc000d000550000000000000000000000000000055000000000000000000000000000005500000000000000000000000000000555
55507000000006000e0e0000c000d000550000000000000000000000000000055000000000000000000000000000005500000000000000000000000000000555
55500770000066600eee00ccc00ddd00550020002000200002000020000200055002000200020000200002000020005500200020002000020000200002000555
55500000000000000000000000000000550000000000000000000000000000055000000000000000000000000000005500000000000000000000000000000555
55500000000000000000000000000000550000000000000000000000000000055000000000000000000000000000005500000000000000000000000000000555
55500000000000000000000000000000550000000000000000000000000000055000000000000000000000000000005500000000000000000000000000000555
55500770000066000e0000ccc00ddd00550000000000000000000000000000055000000000000000000000000000005500000000000000000000000000000555
55507000000006000e000000c00d0000550000000000000000000000000000055000000000000000000000000000005500000000000000000000000000000555
55507000000006000eee000cc00ddd00550000000000000000000000000000055000000000000000000000000000005500000000000000000000000000000555
55507000000006000e0e0000c0000d00550000000000000000000000000000055000000000000000000000000000005500000000000000000000000000000555
55500770000066600eee00ccc00ddd00550020002000200002000020000200055002000200020000200002000020005500200020002000020000200002000555
55500000000000000000000000000000550000000000000000000000000000055000000000000000000000000000005500000000000000000000000000000555
55500000000000000000000000000000550000000000000000000000000000055000000000000000000000000000005500000000000000000000000000000555
55501111111111111111111111aaaaa0550000000000000000000000000000055000000000000000000000000000005500000000000000000000000000000555
55501771111166111e1111cc11addaa0550000000000000000000000000000055000000000000000000000000000005500000000000000000000000000000555
55507111111116111e11111c11aadaa0550000000000000000000000000000055000000000000000000000000000005500000000000000000000000000000555
55507111111116111eee111c11aadaa0550000000000000000000000000000055000000000000000000000000000005500000000000000000000000000000555
55507111111116111e1e111c11aadaa0550000000000000000000000000000055000000000000000000000000000005500000000000000000000000000000555
55501771111166611eee11ccc1addda0550020002000200002000020000200055002000200020000200002000020005500200020002000020000200002000555
55501111111111111111111111aaaaa0550000000000000000000000000000055000000000000000000000000000005500000000000000000000000000000555
55500000000000000000000000000000550000000000000000000000000000055000000000000000000000000000005500000000000000000000000000000555
55500000000000000000000000000000550000000000000000000000000000055000000000000000000000000000005500000000000000000000000000000555
55500000000000000000000000000000550000000000000000000000000000055000000000000000000000000000005500000000000000000000000000000555
55500000000000000000000000000000550000000000000000000000000000055000000000000000000000000000005500000000000000000000000000000555
55500000000000000000000000000000550000000000000000000000000000055000000000000000000000000000005500000000000000000000000000000555
55500000000000000000000000000000550000000000000000000000000000055000000000000000000000000000005500000000000000000000000000000555
55500200020002000020000200002000550020002000200002000020000200055002000200020000200002000020005500200020002000020000200002000555
55500000000000000000000000000000550000000000000000000000000000055000000000000000000000000000005500000000000000000000000000000555
55500000000000000000000000000000550000000000000000000000000000055000000000000000000000000000005500000000000000000000000000000555
55500000000000000000000000000000550000000000000000000000000000055000000000000000000000000000005500000000000000000000000000000555
55500000000000000000000000000000550000000000000000000000000000055000000000000000000000000000005500000000000000000000000000000555
55500000000000000000000000000000550000000000000000000000000000055000000000000000000000000000005500000000000000000000000000000555
55500000000000000000000000000000550000000000000000000000000000055000000000000000000000000000005500000000000000000000000000000555
55500000000000000000000000000000550000000000000000000000000000055000000000000000000000000000005500000000000000000000000000000555
55500200020002000020000200002000550020002000200002000020000200055002000200020000200002000020005500200020002000020000200002000555
55500000000000000000000000000000550000000000000000000000000000055000000000000000000000000000005500000000000000000000000000000555
55500000000000000000000000000000550000000000000000000000000000055000000000000000000000000000005500000000000000000000000000000555
55500000000000000000000000000000550000000000000000000000000000055000000000000000000000000000005500000000000000000000000000000555
55500000000000000000000000000000550000000000000000000000000000055000000000000000000000000000005500000000000000000000000000000555
55500000000000000000000000000000550000000000000000000000000000055000000000000000000000000000005500000000000000000000000000000555
55500000000000000000000000000000550000000000000000000000000000055000000000000000000000000000005500000000000000000000000000000555
55500000000000000000000000000000550000000000000000000000000000055000000000000000000000000000005500000000000000000000000000000555
55500200020002000020000200002000550020002000200002000020000200055002000200020000200002000020005500200020002000020000200002000555
55500000000000000000000000000000550000000000000000000000000000055000000000000000000000000000005500000000000000000000000000000555
55500000000000000000000000000000550000000000000000000000000000055000000000000000000000000000005500000000000000000000000000000555
55555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555
55555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555
55555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555
55555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555
55555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555
55555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555
55555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555
55555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555
55555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555
55555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555
55555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555
55555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555
55555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555
55555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555
55555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555
55555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555
88888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888
88888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888
88888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888
88888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888
88888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888
88888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888
88888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888

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
011000001a3200e1251a3001a0021a00200301013010c201004011800018000180001800018000180001800018000180001800018000180001800018000180001800018000180001800018000180001800018000
01100000006350c6011c5050e5051d5051c505155051d5051f5000e4011640116401174011840118401044010a401104010e4010c401104010e401184010c4011040117401024010c4010e401024010a40110401
01100000246230c6140c6340c6510c6410c6310c6210c6110c6110c6110c6150c6050c6000c6000c6000c60002200346053460434605286033460534605346050220034605346043460528603346053460534605
0110000002673026630265302643026330262302613026130e6051b2061f206272062d505000001d5052d505285051d5052b5051d505295052b50526505295052850526505295052850526505000002850511700
011000001a5731a5131a5131a305212051c30526405212051f30526405263051f3052120526305294052120502405024050e4050e40502405024050e4050e40502405024050e4050e40502405024050f4050c405
01100000022640223102211022151f2051c305244051f2051f30524405263051f3051f20526305284051f2051a200262051b200000001f200000002420000000282000000024200000001f200000001c20000000
011000081a6111a6111a6111a6111a6111a6111a6111a6111f60124401263051f3051f20526305284051f205022000220002200022000e2000e200266040e2003260302200022000220011200112003260411200
011000001a5031a5051a5031a0031a0031a0031a0031a003024020240202402024020240202402024020240202402024020240202402024020240202402024020240202402024020240202402024020240202402
010c00000024500445003450c4451824500445003450c4450024500445003450c4451824500445003450c4450024500445003450c4451824500445003450c4450024500445003450c44518245004450c34500445
010c00000a2450a4450a34516445222450a4450a345164450a2450a4450a34516445222450a4450a345164450a2450a4450a34516445222450a4450a345164450a2450a4450a34516445222450a445163450a445
010c00000824508445083451444520245084450834514445082450844508345144452024508445083451444508245084450834514445202450844508345144450824508445083451444520245084451434508445
010c0000052450544505345114451d245054450534511445052450544505345114451d245054450534511445072450744507345134451f245074450734513445072450744507345134451f245074451334507445
010c00000c2450c4450c34518445242450c4450c345184450c2450c4450c34518445242450c4450c345184450c2450c4450c34518445242450c4450c345184450c2450c4450c34518445242450c445183450c445
010c000024500245002450024500185001f5001f500295001f50000000205000000000000000001f5000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
010c00200c3350c3351f2250c23530613246141b335246030c2350c335184350c2353061324614002350c6140c3350c335004020c2353061324614183350c6140c2350c2351f2250c3353061324614002350c604
010c00000d3350d335112250d23530613256131b335246030c2350c335184350c235306132461300235032040c3350c335004020c235306132461318335246030c2350c2351f2250c33530613246130023524000
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

