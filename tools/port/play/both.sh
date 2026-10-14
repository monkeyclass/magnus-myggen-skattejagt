#!/bin/sh
# usage: both.sh WAIT_MS cmd... (click:x,y down:x,y up:x,y move:x,y rclick:x,y esc) -> side-by-side screenshot path
export DISPLAY=:5; P=/tmp/claude-0/play/ng; w=$1; shift
for c in "$@"; do
  k=${c%%:*}; a=${c#*:}; x=${a%,*}; y=${a#*,}
  case $k in
    click) xdotool mousemove $x $y; echo pmove:$x,$y >> $P/ctl; sleep 0.3; xdotool mousedown 1; sleep 0.15; xdotool mouseup 1; echo pclick:$x,$y >> $P/ctl;;
    move) xdotool mousemove $x $y; echo pmove:$x,$y >> $P/ctl;;
    rclick) xdotool mousemove $x $y; echo pmove:$x,$y >> $P/ctl; sleep 0.3; xdotool click 3; echo prclick:$x,$y >> $P/ctl;;
    esc) xdotool key Escape; echo esc:0,0 >> $P/ctl;;
  esac
  sleep 0.4
done
sleep $(echo "$w/1000" | bc -l)
n=$(cat /tmp/claude-0/cmp/bn 2>/dev/null || echo 0); n=$((n+1)); echo $n > /tmp/claude-0/cmp/bn
import -window root -crop 640x480+0+0 /tmp/claude-0/cmp/bo$n.png; echo shot:$((300+n)) >> $P/ctl; sleep 0.6
python3 -c "
from PIL import Image
a=Image.open('/tmp/claude-0/cmp/bo$n.png'); b=Image.open('$P/shot$((300+n)).png'); o=Image.new('RGB',(1280,480)); o.paste(a,(0,0)); o.paste(b,(640,0)); o.save('/tmp/claude-0/cmp/b$n.png')"
echo /tmp/claude-0/cmp/b$n.png
