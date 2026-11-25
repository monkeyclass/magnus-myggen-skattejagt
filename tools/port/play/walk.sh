#!/bin/sh
# usage: walk.sh ROOM STEPS -> in both, clicks the first clickable target of our room STEPS times; side images w<i>.png
r=$1; P=/tmp/claude-0/play/c$r; export DISPLAY=:5
for i in $(seq 1 $2); do
  n0=$(grep -c "^target" $P/log); echo targets:0,0 >> $P/ctl; sleep 1
  t=$(grep "^target" $P/log | tail -n +$((n0+1)) | grep -v " 0 0$" | grep -v " 319 457$" | grep -v "\.2917 " | tail -1); x=$(echo $t | cut -d' ' -f3); y=$(echo $t | cut -d' ' -f4)
  [ -z "$x" ] && { echo "step $i: no target"; break; }
  PN=c$r /tmp/claude-0/cmp/both.sh 5000 click:$x,$y > /tmp/claude-0/cmp/w.out; cp $(cat /tmp/claude-0/cmp/w.out) /tmp/claude-0/cmp/w$i.png
  echo "step $i: $t"
done
