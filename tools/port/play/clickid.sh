#!/bin/sh
# usage: clickid.sh ROOM OBJ CHILD WAIT -> clicks the centre of that child (from our dump) in both
r=$1; P=/tmp/claude-0/play/c$r
n0=$(grep -c "^visible\|^target" $P/log); echo targets:0,0 >> $P/ctl; echo dump:0,0 >> $P/ctl; sleep 1
t=$(tail -n +1 $P/log | grep "^target $2\.$3 " | tail -1)
if [ -n "$t" ]; then x=$(echo $t | cut -d' ' -f3); y=$(echo $t | cut -d' ' -f4); else
  b=$(grep "^visible $2\.$3 " $P/log | tail -1 | sed 's/.*box //'); x=$(echo $b | awk -F'[,-]' '{print int(($1+$3)/2)}'); y=$(echo $b | awk -F'[,-]' '{print int(($2+$4)/2)}'); fi
[ -z "$x" ] && { echo "no $2.$3"; exit 1; }
echo "$2.$3 at $x,$y"
PN=c$r /tmp/claude-0/cmp/both.sh $4 click:$x,$y
