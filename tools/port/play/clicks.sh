#!/bin/sh
# usage: clicks.sh ROOM -> after room.sh ROOM, clicks each clickable target (from our dump) in both games,
# 4.5 s apart, screenshots co<i>.png / cn<i>.png in /tmp/claude-0/cmp/ROOM
r=$1; d=/tmp/claude-0/cmp/$r; P=/tmp/claude-0/play/c$r; export DISPLAY=:5
echo dump:0,0 >> $P/ctl; sleep 1
grep "^visible" $P/log | awk -v r=$r '{split($2,a,"."); if (($8 ~ /^(1|5|81|101|105|181|185|401|405)$/) ) print $2, $12, $13}' > /dev/null
python3 - $P/log > $d/targets.txt <<'PY'
import sys,re
seen=set(); out=[]
for l in open(sys.argv[1]):
    m=re.match(r'visible (\d+)\.(\d+) type (\w+) z (-?\d+) flags (\w+) box (-?\d+),(-?\d+)-(-?\d+),(-?\d+)',l)
    if not m: continue
    o,c,t,z,f,x0,y0,x1,y1=m.groups(); f=int(f,16)
    if not f&1 or 8<=int(o)<=13: continue
    if x1==x0 or y1==y0: continue
    p=((int(x0)+int(x1))//2,(int(y0)+int(y1))//2)
    if p[1]>=415 or (o,c) in seen: continue
    seen.add((o,c)); out.append('%s.%s %d %d'%(o,c,p[0],p[1]))
print('\n'.join(out))
PY
i=0
while read id x y; do
  i=$((i+1))
  xdotool mousemove $x $y; sleep 0.3; xdotool mousedown 1; sleep 0.15; xdotool mouseup 1
  echo pclick:$x,$y >> $P/ctl
  sleep 4.5
  import -window root -crop 640x480+0+0 $d/co$i.png; echo shot:$((100+i)) >> $P/ctl; sleep 0.5
  mv $P/shot$((100+i)).png $d/cn$i.png 2>/dev/null
  echo "$i $id $x,$y"
done < $d/targets.txt
