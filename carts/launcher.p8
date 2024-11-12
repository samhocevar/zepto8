pico-8 cartridge // http://www.pico-8.com
version 41
__lua__

--------------
-----WIDE-----
--------------

poke(0x5f36,1)

_cam_x=0
_cam_y=0

_realcamera=camera
function camera(_cx,_cy)
	_cam_x=_cx and _cx or 0
	_cam_y=_cy and _cy or 0
	_realcamera(_cam_x-64,_cam_y)
end

function _widedraw(_f,...)
	_realcamera(_cam_x-64,_cam_y)
	_f(...)
	_map_display(1)
	_realcamera(_cam_x+64,_cam_y)
	_f(...)
	_realcamera(_cam_x,_cam_y)
	_map_display(0)
end

_realcls=cls function cls(...)_widedraw(_realcls,...)end
_realspr=spr function spr(...)_widedraw(_realspr,...)end
_realsspr=sspr function sspr(...)_widedraw(_realsspr,...)end
_realmap=map function map(...)_widedraw(_realmap,...)end
_realrect=rect function rect(...)_widedraw(_realrect,...)end
_realrectfill=rectfill function rectfill(...)_widedraw(_realrectfill,...)end
_realcircfill=circfill function circfill(...)_widedraw(_realcircfill,...)end
_realcirc=circ function circ(...)_widedraw(_realcirc,...)end
_realprint=print function print(...)_widedraw(_realprint,...)end
_realpset=pset function pset(...)_widedraw(_realpset,...)end
_realline=line function line(...)_widedraw(_realline,...)end
--------------

cartdata("z8_launcher_label")

piledir={}
curdir=""
curlist={}
meta_loaded=false

function find(t,si,c)
	for i=si,#t do
		if t[i]==c then
			return i
		end
	end
	return -1
end

function find_end(t,si,c)
	for i=#t-si,1,-1 do
		if t[i]==c then
			return i
		end
	end
	return -1
end

function uplist()
	curdir="/"
	for i=1,#piledir do
		curdir..=piledir[i].."/"
	end
	curlist=dir(curdir)
	add(curlist,"../",1)
	sel=2
end

function get_start_dir()
	curlist={}
	local start=stat(124)
	local st=2
	while st<#start do
		local v=find(start,st,"/")
		if v<0 then
			v=#start+1
		end
		add(piledir,sub(start,st,v-1))
		st=v+1
	end
end	

get_start_dir()
uplist()

function load_meta()
	if not meta_loaded then
		memset(0,0,0x2000)
		if #curlist>0 and sel<=#curlist then
			local dd=curlist[sel]
			extcmd("z8_load_metadata "..curdir..dd)
		end
		meta_loaded = true
	end
end

function back_folder()
	if #piledir>0 and piledir[#piledir]!=".." then
		deli(piledir,#piledir)
	end
end

function _update()
	if btnp(2) then sel-=1 meta_loaded=false end
	if btnp(3) then sel+=1 meta_loaded=false end
	if #curlist>0 then
		sel=(sel-1)%#curlist+1
	else
		sel=1
	end
	if btnp(5) then
		meta_loaded=false
		back_folder()
		uplist()
	end
	if btnp(4) then
		if #curlist>0 and sel<=#curlist then
			meta_loaded=false
			local dd=curlist[sel]
			local isdir=dd[#dd]=="/"
			if isdir then
				if dd=="../" then
					back_folder()
				else
					add(piledir,sub(dd,1,#dd-1))
				end
				uplist()
			else
				load(curdir..dd,"back to launcher")
			end
		end
	end
end

function bprint(t,x,y,c)
	print(t,x-1,y,1)
	print(t,x+1,y,1)
	print(t,x,y-1,1)
	print(t,x,y+1,1)
	print(t,x,y,c)
end

function _draw()
	load_meta()

	camera()
	cls(1)
	print(curdir,65,1,7)
	local base=min(max(sel-10,1),max(1,#curlist-18))
	for i=base,min(base+19,#curlist) do
		local dd=curlist[i]
		local isdir=dd[#dd]=="/"
		local c=isdir and 4 or 13
		if(i==sel) c=isdir and 8 or 12
		print(dd,65,(i-base+1)*6+1,c)
	end

	camera(64,0)
	rectfill(0,0,127,127,0)
	spr(0,0,0,16,16)
	bprint(stat(130),4,4,7)
	bprint(stat(131),4,10,7)
end
__gfx__
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00700700000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00077000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00077000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00700700000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
