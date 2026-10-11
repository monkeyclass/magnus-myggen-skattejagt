#!/bin/sh
# Copy the game files from the Nordic CD into the app's game folder.
#
# usage: prepare-game.sh <disc> <game-dir> [DK|N|S|SF]
#   <disc>      the CD-ROM mount point, an extracted copy, or an .iso
#   <game-dir>  e.g. /media/SDCARD/App/MagnusMyggen2/game
#   language    default DK; the other language folders are left out to save space
#
# Needs unshield (and 7z for an .iso).
set -eu

disc=$1
game=$2
lang=${3:-DK}
command -v unshield >/dev/null || { echo "error: unshield is not installed" >&2; exit 1; }

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
if [ -f "$disc" ]; then
	echo "Extracting $disc ..."
	7z x -y -o"$work/disc" "$disc" >/dev/null
	disc="$work/disc"
fi
find_ci() { find "$disc" -maxdepth 1 -iname "$1" -print -quit; }

mkdir -p "$game"
for f in MM2.DAT MM2.IDX; do
	p=$(find_ci "$f"); [ -n "$p" ] || { echo "error: $f not found on $disc" >&2; exit 1; }
	echo "Copying $f ..."
	cp "$p" "$game/$f"
done
for d in "$lang" LAB; do
	p=$(find_ci "$d"); [ -n "$p" ] || { echo "error: $d/ not found on $disc" >&2; exit 1; }
	echo "Copying $d/ ..."
	rm -rf "$game/$d"
	cp -r "$p" "$game/$d"
done

cab=$(find_ci DATA1.CAB)
echo "Unpacking mm2run.exe from $(basename "$cab") ..."
unshield -d "$work/cab" x "$cab" >/dev/null
cp "$work/cab/Program_files_$lang/mm2run.exe" "$game/mm2run.exe"
echo "Done."
