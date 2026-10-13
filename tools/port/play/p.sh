#!/bin/sh
# usage: p.sh NAME WAIT_MS cmd... -> runs commands, waits, takes a screenshot (prints its path)
d=/tmp/claude-0/play/$1; w=$2; shift 2
for c in "$@"; do echo "$c" >> $d/ctl; done
sleep $(echo "$w/1000" | bc -l)
n=$(cat $d/n 2>/dev/null || echo 0); n=$((n+1)); echo $n > $d/n
echo "shot:$n" >> $d/ctl; sleep 0.5
echo $d/shot$n.png
