#!/usr/bin/env bash
# Install "Magnus & Myggen: Den store skattejagt" (Ivanoff Interactive,
# Nordic retail CD, 2001/2002 build) into a Wine or Proton prefix without
# running the InstallShield setup, which tends to hang under Wine.
#
# usage: install-mm2.sh <disc> [--lang DK|N|S|SF] [--prefix DIR] [--proton PATH] [--no-wine-fix]
#   <disc>      the CD-ROM mount point, an extracted copy of it, or an .iso
#   --lang      game language (default DK)
#   --prefix    Wine prefix (default ~/.local/share/mm2/prefix).
#               With --proton this is STEAM_COMPAT_DATA_PATH; the Wine
#               prefix itself is <prefix>/pfx.
#   --proton    path to a Proton build's "proton" script (e.g. GE-Proton)
#   --no-wine-fix
#               don't install the patched winmm.dll/iccvid.dll from wine-fix/;
#               without them the labyrinth videos stay black (see wine-fix/README.md)
#
# Needs: unshield (and 7z when <disc> is an .iso).
set -euo pipefail

die() { echo "error: $*" >&2; exit 1; }

disc="" lang="DK" prefix="$HOME/.local/share/mm2/prefix" proton="" winefix=1
here=$(cd "$(dirname "$0")" && pwd)
while [ $# -gt 0 ]; do
	case "$1" in
	--lang) lang="${2^^}"; shift 2 ;;
	--prefix) prefix="$2"; shift 2 ;;
	--proton) proton="$2"; shift 2 ;;
	--no-wine-fix) winefix=0; shift ;;
	-h|--help) sed -n '2,18p' "$0"; exit 0 ;;
	*) [ -z "$disc" ] || die "unexpected argument: $1"; disc="$1"; shift ;;
	esac
done
[ -n "$disc" ] || die "no disc given (see --help)"
case "$lang" in DK|N|S|SF) ;; *) die "language must be DK, N, S or SF" ;; esac
command -v unshield >/dev/null || die "unshield is not installed"

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

if [ -f "$disc" ]; then
	command -v 7z >/dev/null || die "7z is needed to read an .iso"
	echo "Extracting $disc ..."
	7z x -y -o"$work/disc" "$disc" >/dev/null
	disc="$work/disc"
fi

# Find a file on the disc regardless of case (ISO extractors differ).
find_ci() { find "$disc" -maxdepth 2 -ipath "$disc/$1" -print -quit; }
cab=$(find_ci DATA1.CAB)
[ -n "$cab" ] || die "DATA1.CAB not found on $disc - is this the right CD?"

echo "Unpacking the program files ..."
unshield -d "$work/cab" x "$cab" >/dev/null
exe="$work/cab/Program_files_$lang/mm2run.exe"
[ -f "$exe" ] || die "mm2run.exe for language $lang is missing from the cabinet"

if [ -n "$proton" ]; then
	[ -x "$proton" ] || die "$proton is not executable"
	export STEAM_COMPAT_DATA_PATH="$prefix"
	export STEAM_COMPAT_CLIENT_INSTALL_PATH="${STEAM_COMPAT_CLIENT_INSTALL_PATH:-$prefix}"
	mkdir -p "$prefix"
	run() { "$proton" run "$@"; }
	pfx="$prefix/pfx"
else
	command -v wine >/dev/null || die "wine is not installed"
	export WINEPREFIX="$prefix"
	run() { wine "$@"; }
	pfx="$prefix"
fi

echo "Initialising the prefix at $pfx ..."
run wineboot -u >/dev/null 2>&1 || true
[ -d "$pfx/drive_c" ] || die "prefix was not created at $pfx"

game="$pfx/drive_c/Program Files/Magnus2"
echo "Copying the game to $game ..."
mkdir -p "$game/$lang" "$game/Lab"
cp "$exe" "$game/mm2run.exe"
cp "$work/cab/Program_files/"*.ico "$game/" 2>/dev/null || true
cp "$(find_ci MM2.DAT)" "$game/mm2.dat"
cp "$(find_ci MM2.IDX)" "$game/mm2.idx"
cp "$(find_ci "$lang/MM2LNG.DAT")" "$game/$lang/mm2lng.dat"
cp "$(find_ci "$lang/MM2LNG.IDX")" "$game/$lang/mm2lng.idx"
labdir=$(find "$disc" -maxdepth 1 -type d -iname lab -print -quit)
[ -n "$labdir" ] || die "Lab directory not found on $disc"
cp "$labdir"/* "$game/Lab/"

# The same values the original installer writes. /reg:32 puts them where a
# 32-bit program looks in a 64-bit prefix (Wow6432Node); it is harmless in
# a 32-bit prefix.
echo "Writing registry settings ..."
key='HKLM\Software\IVANOFF Interactive\mm2'
dir='C:\Program Files\Magnus2'
regadd() { run reg add "$key" /v "$1" /t REG_SZ /d "$2" /f /reg:32 >/dev/null 2>&1; }
regadd LabPath "$dir\\Lab\\"
regadd Language "$lang"
regadd ProgramPath "$dir\\"
regadd "Resource file" "$dir\\mm2"
regadd "Resource local file" "$dir\\$lang\\mm2lng"

# The game draws on the Windows desktop itself rather than in its own
# window, so without a Wine virtual desktop the screen stays black.
# This applies to the whole prefix, which is dedicated to the game anyway.
echo "Enabling a 640x480 virtual desktop for this prefix ..."
run reg add 'HKCU\Software\Wine\Explorer' /v Desktop /t REG_SZ /d Default /f >/dev/null 2>&1
run reg add 'HKCU\Software\Wine\Explorer\Desktops' /v Default /t REG_SZ /d 640x480 /f >/dev/null 2>&1

# Wine's winmm and Cinepak decoder can't play the labyrinth videos (black
# screen); load the fixed copies from the game folder for this game only.
if [ "$winefix" = 1 ]; then
	[ -f "$here/wine-fix/winmm.dll" ] && [ -f "$here/wine-fix/iccvid.dll" ] ||
		die "wine-fix/winmm.dll and iccvid.dll not found next to this script (or use --no-wine-fix)"
	echo "Installing the labyrinth video fix (patched winmm.dll, iccvid.dll) ..."
	cp "$here/wine-fix/winmm.dll" "$here/wine-fix/iccvid.dll" "$game/"
	okey='HKCU\Software\Wine\AppDefaults\mm2run.exe\DllOverrides'
	run reg add "$okey" /v winmm /t REG_SZ /d native,builtin /f >/dev/null 2>&1
	run reg add "$okey" /v iccvid /t REG_SZ /d native,builtin /f >/dev/null 2>&1
fi

echo
echo "Installed. Start the game with:"
if [ -n "$proton" ]; then
	echo "  STEAM_COMPAT_DATA_PATH='$prefix' STEAM_COMPAT_CLIENT_INSTALL_PATH='$STEAM_COMPAT_CLIENT_INSTALL_PATH' \\"
	echo "    '$proton' run '$game/mm2run.exe'"
else
	echo "  WINEPREFIX='$prefix' wine '$game/mm2run.exe'"
fi
