#!/bin/sh
# Build the Onion OS app for the Miyoo Mini.
#
# usage: build.sh <scummvm-source> [<out-dir>]
#   <scummvm-source>  a ScummVM source tree (git master; tested with October 2026)
#   <out-dir>         where to put App/MagnusMyggen2 (default: ./dist)
#
# Needs the Miyoo Mini toolchain in /opt/miyoomini-toolchain:
#   https://github.com/shauninman/miyoomini-toolchain-buildroot/releases (v0.0.3)
set -eu

src=$(cd "$1" && pwd)
out=${2:-dist}
mkdir -p "$out"
out=$(cd "$out" && pwd)
here=$(cd "$(dirname "$0")" && pwd)
tc=/opt/miyoomini-toolchain
[ -d "$tc" ] || { echo "error: $tc is missing" >&2; exit 1; }
export PATH="$tc/usr/bin:$tc/bin:$PATH"
export CXX=arm-linux-gnueabihf-g++

echo "Copying the engine into $src ..."
rm -rf "$src/engines/ivanoff"
cp -r "$here/../scummvm/engines/ivanoff" "$src/engines/ivanoff"

cd "$src"
./configure --host=miyoomini --enable-release --disable-all-engines --enable-engine=ivanoff \
	--enable-ext-neon --with-freetype2-prefix="$tc/arm-linux-gnueabihf/libc/usr/bin/"
make -j"$(nproc)"
rm -rf sd-root
make sd-root

cd "$here"
app="$out/App/MagnusMyggen2"
rm -rf "$app"
mkdir -p "$app/lib" "$app/game"
cp "$src/sd-root/App/scummvm/scummvm" "$app/"
cp -r "$src/sd-root/App/scummvm/themes" "$src/sd-root/App/scummvm/engine-data" "$app/"
cp "$src/sd-root/App/scummvm/COPYING" "$src/sd-root/App/scummvm/COPYRIGHT" "$app/"
cp "$src/sd-root/App/scummvm/scummvm.png" "$app/icon.png"
# Libraries the firmware may lack (SDL comes from the firmware).
sysroot="$tc/arm-linux-gnueabihf/libc/usr/lib"
cp "$sysroot/libfreetype.so.6" "$app/lib/"
cp "$sysroot/libpng16.so.16" "$app/lib/"
cp "$sysroot/libz.so.1" "$app/lib/"
cp app/launch.sh app/config.json app/README.txt "$app/"
cp app/PUT_GAME_FILES_HERE.txt "$app/game/"
echo "Done: $app ($(du -sh "$app" | cut -f1)). Add the game files to $app/game."
