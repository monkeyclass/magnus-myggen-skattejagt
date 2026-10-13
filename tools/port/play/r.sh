#!/bin/sh
# usage: r.sh WAIT_MS cmd... (click:x,y rclick:x,y move:x,y key:Name) -> screenshot path of the original under Wine
export DISPLAY=:5
w=$1; shift
for c in "$@"; do
  k=${c%%:*}; a=${c#*:}; x=${a%,*}; y=${a#*,}
  case $k in
    click) xdotool mousemove $x $y; sleep 0.3; xdotool mousedown 1; sleep 0.15; xdotool mouseup 1;;
    rclick) xdotool mousemove $x $y; sleep 0.1; xdotool click 3;;
    move) xdotool mousemove $x $y;;
    key) xdotool key $a;;
  esac
  sleep 0.2
done
sleep $(echo "$w/1000" | bc -l)
n=$(cat /tmp/claude-0/ref/n 2>/dev/null || echo 1); n=$((n+1)); echo $n > /tmp/claude-0/ref/n
import -window root -crop 640x480+0+0 /tmp/claude-0/ref/r$n.png; echo /tmp/claude-0/ref/r$n.png
