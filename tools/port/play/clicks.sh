#!/bin/sh
# usage: clicks.sh ROOM -> after room.sh ROOM, clicks each clickable target (from our dump) in both games,
# 4.5 s apart, screenshots co<i>.png / cn<i>.png in /tmp/claude-0/cmp/ROOM
r=$1; d=/tmp/claude-0/cmp/$r; P=/tmp/claude-0/play/c$r; export DISPLAY=:5
echo targets:0,0 >> $P/ctl; sleep 1
grep "^target " $P/log | awk '$4 < 415 {print $2, $3, $4}' | awk '!seen[$2" "$3]++' > $d/targets.txt
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
