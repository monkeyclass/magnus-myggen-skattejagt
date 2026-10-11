#!/bin/bash
# Checks native ports against the original code running in the interpreter.
#
# usage: portcheck.sh TREE NAME VTABLES
#   TREE     a ScummVM tree with the engine built (e.g. /home/user/agents/a)
#   NAME     prefix for the logs, in /tmp/claude-0/det/NAME/
#   VTABLES  the classes to check, comma separated hex (e.g. 4b4944,4b49e4)
#
# 1. the room and walk runs, compared with the baselines of the original
# 2. targeted fuzzing: messages and variable changes for the given classes,
#    with the native logic and without it (native_scripts=none)
# Prints the first difference of each run that differs, and the coverage of
# the classes' handlers by the fuzzing runs.
tree=$1; name=$2; vts=$3
out=/tmp/claude-0/det/$name; rm -rf $out; mkdir -p $out
det=/tmp/claude-0/det
run() { # run NAME ROOM SEED EVENTS [ini lines]
	n=$1; r=$2; seed=$3; ev=$4; shift 4
	rm -rf $out/saves-$n; mkdir -p $out/saves-$n
	python3 /home/user/magnus-myggen-skattejagt/tools/m2s_setroom.py /home/user/analysis/hall-save.m2s $out/saves-$n/mm2-dk-SavedGame1.m2s $r
	printf '[SETTINGS]\r\nSoundLevel=240\r\nSavedGame1=15\r\nSavedGameTime1=0\r\n' > $out/saves-$n/mm2-dk-settings.dat
	{ printf '[scummvm]\ngfx_mode=surfacesdl\nenable_unsupported_game_warning=false\nsavepath=%s\nextrapath=/home/user/scummvm/dists/engine-data\nthemepath=/home/user/scummvm/gui/themes\n' $out/saves-$n
	  printf '[mm2-dk]\nengineid=ivanoff\ngameid=mm2\nlanguage=da\nplatform=windows\npath=/home/user/gamedata/mm2-scummvm\nrandom_seed=%s\nvirtual_clock=20\ndebug_events=%s\n' $seed "$ev"
	  for l in "$@"; do echo "$l"; done; } > $out/$n.ini
	SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy timeout 300 $tree/scummvm -c $out/$n.ini -d4 mm2-dk 2>&1 | grep -v "Interpreter:\|^Using configuration\|native logic" > $out/$n.log
}
export -f run 2>/dev/null
ROOMEV="8000:esc 20000:click:80,58 23000:click:160,457 36000:crawl:40,3000 160000:quit"
FUZZEV="8000:esc 20000:click:80,58 23000:click:160,457 36000:fuzz:1500,60 40000:crawl:30,3000 140000:quit"
jobs=""
i=0
# The fuzzing also starts in the rooms of the checked classes (room objects have the room's id).
FROOMS=$(for v in $(echo $vts | tr ',' ' '); do grep "vt=$v " /home/user/analysis/decomp/rooms.txt | awk '$1 ~ /^[0-9]+$/ && $1 >= 15 && $1 <= 103 {print $1}'; done | sort -un | tr '\n' ' ')
for r in 22 24 27 34 36 38 43 44 49 52 55 18 40 46 47 15 16 59 $FROOMS; do
	i=$((i+1))
	( run room$r $r 1234 "$ROOMEV" native_scripts=$vts ) &
	( run nf$r $r $((r*7)) "$FUZZEV" native_scripts=$vts fuzz_dict=/home/user/analysis/decomp/fuzz.dict fuzz_vtables=$vts fuzz_vars=$([ $((i%2)) = 0 ] && echo true || echo false) ) &
	( run ef$r $r $((r*7)) "$FUZZEV" native_scripts=none fuzz_dict=/home/user/analysis/decomp/fuzz.dict fuzz_vtables=$vts fuzz_vars=$([ $((i%2)) = 0 ] && echo true || echo false) coverage=$out/ef$r.cov ) &
	[ $((i%2)) = 0 ] && wait
done
wait
same=0; diff=0
for r in 22 24 27 34 36 38 43 44 49 52 55 18 40 46 47 15 16 59 $FROOMS; do
	[ -f $det/base$r.log ] || continue
	if grep -v "^Using configuration\|native logic" $det/base$r.log | cmp -s - $out/room$r.log; then same=$((same+1)); else diff=$((diff+1)); echo "== room $r: first difference (< original, > native):"; grep -v "^Using configuration\|native logic" $det/base$r.log | diff - $out/room$r.log | head -8; fi
	if cmp -s $out/ef$r.log $out/nf$r.log; then same=$((same+1)); else diff=$((diff+1)); echo "== fuzz $r: first difference (< original, > native):"; diff $out/ef$r.log $out/nf$r.log | head -8; fi
done
echo "runs: same $same, different $diff"
echo "coverage of the classes' handlers by the fuzzing:"
python3 /home/user/magnus-myggen-skattejagt/tools/port/coverage.py /home/user/analysis/decomp/game.asm /home/user/analysis/decomp/rooms.txt /home/user/analysis/decomp/typed.c $out/ef*.cov | grep -E "vt=($(echo $vts | tr ',' '|')) "
echo "logs: $out (ef* original, nf* native, room* native vs $det/base*)"
