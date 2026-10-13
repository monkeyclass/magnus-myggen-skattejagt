#!/bin/sh
# usage: room.sh ROOM -> loads the same save (room ROOM) in the original (Wine) and in ours, takes
# 12 screenshots 500 ms apart in each: /tmp/claude-0/cmp/ROOM/{o,n}NN.png
r=$1; d=/tmp/claude-0/cmp/$r; rm -rf $d; mkdir -p $d
W=/home/user/gamedata/wine-ref; A=$W/drive_c/ProgramData/IVANOFF/MM2/2.0
python3 /home/user/magnus-myggen-skattejagt/tools/m2s_setroom.py /home/user/analysis/hall-save.m2s $d/save.m2s $r
# original
WINEPREFIX=$W wineserver -k 2>/dev/null; sleep 1
cp $d/save.m2s $A/SavedGame1.m2s
printf '[SETTINGS]\r\nSoundLevel=240\r\nSavedGame1=15\r\nSavedGameTime1=0\r\n' > $A/settings.dat
(cd "$W/drive_c/Program Files/Magnus2" && DISPLAY=:5 WINEPREFIX=$W WINEDEBUG=-all nohup wine mm2run.exe >/dev/null 2>&1 &)
# ours
pkill -x scummvm; sleep 1
/tmp/claude-0/play/start.sh c$r >/dev/null
P=/tmp/claude-0/play/c$r
cp $d/save.m2s $P/saves/mm2-dk-SavedGame1.m2s
printf '[SETTINGS]\r\nSoundLevel=240\r\nSavedGame1=15\r\nSavedGameTime1=0\r\n' > $P/saves/mm2-dk-settings.dat
export DISPLAY=:5
sleep 10
xdotool mousemove 320 240; sleep 0.3; xdotool mousedown 1; sleep 0.15; xdotool mouseup 1; sleep 0.3; xdotool key Escape
echo esc:0,0 >> $P/ctl
sleep 5
for xy in "80 58" "160 457"; do
  set -- $xy
  xdotool mousemove $1 $2; sleep 0.3; xdotool mousedown 1; sleep 0.15; xdotool mouseup 1
  echo pclick:$1,$2 >> $P/ctl
  sleep 2
done
xdotool mousemove 632 8; echo pmove:632,8 >> $P/ctl
sleep ${SETTLE:-8}
for i in $(seq -w 1 12); do
  import -window root -crop 640x480+0+0 $d/o$i.png
  echo shot:$i >> $P/ctl
  sleep 0.5
done
sleep 1
for i in $(seq -w 1 12); do mv $P/shot$(expr $i + 0).png $d/n$i.png 2>/dev/null; done
grep "msg 4 -> 17" $P/log | tail -1
