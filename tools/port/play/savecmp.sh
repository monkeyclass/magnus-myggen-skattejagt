#!/bin/sh
# usage: savecmp.sh ROOM -> after room.sh + clicks.sh ROOM, saves slot 1 in both and diffs the records
r=$1; A=/home/user/gamedata/wine-ref/drive_c/ProgramData/IVANOFF/MM2/2.0; P=/tmp/claude-0/play/c$r
rm -f $A/SavedGame1.m2s $P/saves/mm2-dk-SavedGame1.m2s
sleep 6
for c in "3000 click:600,445" "3000 click:195,138" "1500 click:80,58" "4000 click:160,457"; do PN=c$r /tmp/claude-0/cmp/both.sh $c >/dev/null; done
if [ ! -f $A/SavedGame1.m2s ] || [ ! -f $P/saves/mm2-dk-SavedGame1.m2s ]; then echo "$r: no save (o=$(ls $A/SavedGame1.m2s 2>/dev/null) n=$(ls $P/saves/mm2-dk-SavedGame1.m2s 2>/dev/null))"; exit; fi
cp $A/SavedGame1.m2s /tmp/claude-0/cmp/$r/save_o.m2s; cp $P/saves/mm2-dk-SavedGame1.m2s /tmp/claude-0/cmp/$r/save_n.m2s
echo "$r: $(python3 /home/user/magnus-myggen-skattejagt/tools/m2s.py /tmp/claude-0/cmp/$r/save_o.m2s /tmp/claude-0/cmp/$r/save_n.m2s | grep -v '^header' | tr '\n' ';')"
