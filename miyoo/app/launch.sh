#!/bin/sh
# Onion OS launcher for "Magnus & Myggen 2: Den store skattejagt" (ScummVM, ivanoff engine).
progdir=$(cd "$(dirname "$0")" && pwd)
cd "$progdir"
export HOME="$progdir"
export LD_LIBRARY_PATH="$progdir/lib:$LD_LIBRARY_PATH"

# The game language: da (Danish), nb (Norwegian), sv (Swedish) or fi (Finnish).
# Set it in language.txt; the matching folder (DK, N, S or SF) must be in game/.
lang=$(cat "$progdir/language.txt" 2>/dev/null | tr -d ' \r\n')
[ -n "$lang" ] || lang=da

mkdir -p "$progdir/saves"
if [ ! -f "$progdir/scummvm.ini" ] || ! grep -q "^language=$lang\$" "$progdir/scummvm.ini"; then
	cat > "$progdir/scummvm.ini" <<INI
[scummvm]
savepath=$progdir/saves
themepath=$progdir/themes
extrapath=$progdir/engine-data
enable_unsupported_game_warning=false
gui_return_to_launcher_at_exit=false

[mm2]
engineid=ivanoff
gameid=mm2
language=$lang
platform=windows
path=$progdir/game
description=Magnus & Myggen 2: Den store skattejagt
INI
fi

./scummvm -c "$progdir/scummvm.ini" mm2 > "$progdir/log.txt" 2>&1
