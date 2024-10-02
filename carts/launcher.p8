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

function find_carts()
	local allfiles=ls()
	local filterfiles={}
	for i=1,#allfiles do
		local str=allfiles[i]
		if #str>7 and sub(str,#str-6)==".p8.png" then
			add(filterfiles,str)
		elseif #str>3 and sub(str,#str-2)==".p8" then
			add(filterfiles,str)
		else
		end
	end
	return filterfiles
end

files = find_carts()
fidx=max(1,dget(0))
pb4=false
meta_loaded=false

function bprint(t,x,y,c)
	print(t,x-1,y,1)
	print(t,x+1,y,1)
	print(t,x,y-1,1)
	print(t,x,y+1,1)
	print(t,x,y,c)
end

function _update()
	if btn(4) and not pb4 then
		if fidx > 0 and fidx <= #files then
			local str = files[fidx]
			load(str,"back to launcher")
		end
	end
	if #files>0 then
		if btnp(2) then fidx-=1 meta_loaded=false end
		if btnp(3) then fidx+=1 meta_loaded=false end
		fidx = (fidx-1)%#files+1
		dset(0,fidx)
	end
	pb4=btn(4)
end

function _draw()
	if #files>0 then
		if not meta_loaded then
			extcmd("z8_load_metadata "..files[fidx])
			local strlabel = stat(132)
			---[[
			for i=0,128*128-1 do
				local c=0
				if (i<#strlabel) c=tonum("0x"..sub(strlabel,i+1,i+1))
				sset(i%128,flr(i/128),c)
			end
			--]]
			meta_loaded = true
		end
		cls(1)
		camera(-64,mid(64,fidx*6,#files*6-52)-64)
		for i=1,#files do
			print(files[i],1,1+i*6,fidx==i and 7 or 12)	
		end

		camera(64,0)
		rectfill(0,0,127,127,0)
		spr(0,0,0,16,16)
		bprint(stat(130),4,4,7)
		bprint(stat(131),4,10,7)
	else
		cls(1)
		print("no files found",10,64,14)
	end
end
__gfx__
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00700700000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00077000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00077000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00700700000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
