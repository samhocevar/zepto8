pico-8 cartridge // http://www.pico-8.com
version 17
__lua__

local mul = 200

local fillc = 0
local bg = 1
local fg = 7

local dx = { 1, 0, -1, 0,   1, 1, -1, -1,   1, 1, 0, 0 }
local dy = { 0, 1, 0, -1,   -1, 1, 1, -1,   0, 1, 1, 0 }

-- flood fill
function ffill(x,y,c)
  local c0 = pget(x,y)
  if c0==fillc or (c and c!=c0) then return end
  pset(x,y,fillc)
  for i=1,4 do ffill(x+dx[i],y+dy[i],c0) end
end

-- find bottom-left non-transparent char
function finddot()
  for dx=0,8 do
    for dy=7,0,-1 do
      if pget(dx,dy)!=fillc then
        return dx,dy
      end
    end
  end
end

-- build envelope for current char
function trace(sx,sy)
  local path, visited = { {sx,sy+1} }, {}
  local x,y,d = sx,sy,3
  repeat
    add(visited, {x,y})
    add(visited, {x+dy[d+1],y-dx[d+1]})
    local c = pget(x,y)
    local x1,y1 = x+dx[d+1],y+dy[d+1]
    local x2,y2 = x+dx[d+5],y+dy[d+5]
    local x3,y3 = x+dx[d+9],y+dy[d+9]
    if pget(x1,y1) != c then
      add(path,{x3,y3})
      d = (d+1)%4 -- turn right
    elseif pget(x2,y2) == c then
      add(path,{x3,y3})
      x,y,d = x2,y2,(d+3)%4 -- turn left
    else
      x,y = x1,y1
    end
  until x==sx and y==sy and d==3
  return path,visited
end

function output(path,rev)
  for i=1,#path do
    local p = path[rev and #path-i+1 or i]
    printh((i==1 and '' or ' ')..
           (p[1]*mul-mul).." "..
           (6*mul-p[2]*mul).." "..
           (i==1 and 'm' or 'l').." 1,"..
           ((i-1)%(#path-1))..",-1")
  end
end

function dochar(n)
  local ch = chr(n)
  if band(n,0xdf)>=0x41 and band(n,0xdf)<=0x5a then n=bxor(n,0x20) end
  local w = 4 + 4*flr(n/128)
  local h = 6

  printh('StartChar: u'..sub(tostr(n,1),3,6))
  printh('Encoding: '..n..' '..n..' '..(n - 13))
  printh('Width: '..(w*mul))
  printh('VWidth: '..(6*mul))
  printh('GlyphClass: 2\nFlags: W\nLayerCount: 2\nFore\nSplineSet')

  rectfill(0,0,7,7,bg)
  print(ch,1,1,fg)
  ffill(0,0)
  while true do
    local sx,sy = finddot()
    if not sx then break end
    path,visited = trace(sx,sy)
    output(path,pget(sx,sy)!=fg)
    for _,p in pairs(visited) do
      ffill(p[1],p[2])
    end
  end

  printh('EndSplineSet\nEndChar\n')
end

printh('SplineFontDB: 3.0')
printh('FontName: zepto8\nFullName: ZEPTO-8\nFamilyName: zepto8\nWeight: Book')
printh('Copyright: Sam Hocevar\n')
printh('Version: 1.0\n')
printh('ItalicAngle: 0')
printh('UnderlinePosition: -200\nUnderlineWidth: 200')
printh('Ascent: 1000\nDescent: 0')
printh('LayerCount: 2\nLayer: 0 1 "Back" 1\nLayer: 1 1 "Fore" 0\n')
printh('OS2Version: 2\nOS2_WeightWidthSlopeOnly: 0\nOS2_UseTypoMetrics: 0')
printh('PfmFamily: 81')
printh('TTFWeight: 400\nTTFWidth: 5')
printh('LineGap: 0\nVLineGap: 0\nPanose: 0 0 4 0 0 0 0 0 0 0\n')
printh('OS2TypoAscent: 1000\nOS2TypoAOffset: 0\nOS2TypoDescent: -200\nOS2TypoDOffset: 0\nOS2TypoLinegap: 0')
printh('OS2WinAscent: 1000\nOS2WinAOffset: 0\nOS2WinDescent: -200\nOS2WinDOffset: 0')
printh('HheadAscent: 1000\nHheadAOffset: 0\nHheadDescent: -200\nHheadDOffset: 0')
printh('OS2SubXSize: 400\nOS2SubYSize: 400\nOS2SubXOff: 0\nOS2SubYOff: 0')
printh('OS2SupXSize: 400\nOS2SupYSize: 400\nOS2SupXOff: 0\nOS2SupYOff: 400')
printh('OS2StrikeYSize: 40\nOS2StrikeYPos: 160')
printh('OS2CapHeight: 1000\nOS2XHeight: 800')
printh('GaspTable: 1 65535 2 0\n')
printh('Encoding: UnicodeBmp\n')
printh('DisplaySize: -48')
printh('AntiAlias: 1')
printh('FitToEm: 0')
printh('BeginChars: 65539 125')
for n=0x10,0xff do
  dochar(n)
end
printh('EndChars\nEndSplineFont')

