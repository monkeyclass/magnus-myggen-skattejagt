#!/bin/sh
# usage: start.sh NAME -> starts a real-time game in the background, controlled by NAME/ctl
d=/tmp/claude-0/play/$1; rm -rf $d; mkdir -p $d/saves; : > $d/ctl
cat > $d/ini <<INI
[scummvm]
gfx_mode=surfacesdl
enable_unsupported_game_warning=false
savepath=$d/saves
extrapath=/home/user/scummvm/dists/engine-data
themepath=/home/user/scummvm/gui/themes
[mm2-dk]
engineid=ivanoff
gameid=mm2
language=da
platform=windows
path=/home/user/gamedata/mm2-scummvm
random_seed=1234
debug_control=$d/ctl
INI
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=${AUDIO:-pulseaudio} nohup ${BIN:-/home/user/scummvm/scummvm} -c $d/ini -d5 mm2-dk > $d/log 2>&1 &
echo started
