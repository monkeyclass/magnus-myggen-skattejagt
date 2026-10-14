#!/bin/sh
# starts a new game in both (no saves), waits in the hall
W=/home/user/gamedata/wine-ref; A=$W/drive_c/ProgramData/IVANOFF/MM2/2.0
WINEPREFIX=$W wineserver -k 2>/dev/null; pkill -x scummvm; sleep 1
rm -f $A/SavedGame*.m2s; printf '[SETTINGS]\r\nSoundLevel=240\r\n' > $A/settings.dat
(cd "$W/drive_c/Program Files/Magnus2" && DISPLAY=:5 WINEPREFIX=$W WINEDEBUG=-all nohup wine mm2run.exe >/dev/null 2>&1 &)
/tmp/claude-0/play/start.sh ng >/dev/null
export DISPLAY=:5; P=/tmp/claude-0/play/ng
sleep 10
xdotool mousemove 320 240; sleep 0.3; xdotool mousedown 1; sleep 0.15; xdotool mouseup 1; sleep 0.3; xdotool key Escape; echo esc:0,0 >> $P/ctl
sleep 4; xdotool key Escape; echo esc:0,0 >> $P/ctl
sleep ${SETTLE:-14}
xdotool mousemove 632 8; echo pmove:632,8 >> $P/ctl
