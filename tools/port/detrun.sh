#!/bin/sh
# usage: run.sh NAME ROOM [extra ini lines...] -> deterministic crawl trace in /tmp/claude-0/det/NAME.log
n=$1; r=$2; shift 2
d=/tmp/claude-0/det
rm -rf $d/saves-$n; mkdir -p $d/saves-$n
python3 /home/user/magnus-myggen-skattejagt/tools/m2s_setroom.py /home/user/analysis/hall-save.m2s $d/saves-$n/mm2-dk-SavedGame1.m2s $r
printf '[SETTINGS]\r\nSoundLevel=240\r\nSavedGame1=15\r\nSavedGameTime1=0\r\n' > $d/saves-$n/mm2-dk-settings.dat
{ cat <<INI
[scummvm]
gfx_mode=surfacesdl
enable_unsupported_game_warning=false
savepath=$d/saves-$n
extrapath=/home/user/scummvm/dists/engine-data
themepath=/home/user/scummvm/gui/themes
[mm2-dk]
engineid=ivanoff
gameid=mm2
language=da
platform=windows
path=/home/user/gamedata/mm2-scummvm
random_seed=${SEED:-1234}
virtual_clock=20
debug_events=${EVENTS:-8000:esc 20000:click:80,58 23000:click:160,457 36000:crawl:${CRAWL:-40},3000 ${END:-160000}:quit}
INI
for l in "$@"; do echo "$l"; done; } > $d/$n.ini
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy timeout ${TIMEOUT:-300} /home/user/scummvm/scummvm -c $d/$n.ini -d4 mm2-dk 2>&1 | grep -v "Interpreter:" > $d/$n.log
echo "$n exit $? lines $(wc -l < $d/$n.log) errors: $(grep -cE 'ERROR|Emu:|error' $d/$n.log)"
