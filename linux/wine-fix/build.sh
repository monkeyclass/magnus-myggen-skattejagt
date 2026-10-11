#!/usr/bin/env bash
# Rebuild winmm.dll and iccvid.dll (32-bit) from Wine 10.0 with the fixes in
# patches/wine-10.0, as shipped in this folder.
#
# usage: build.sh [workdir]        (default: ./wine-fix-build)
#
# Needs: git, gcc, flex, bison, i686-w64-mingw32-gcc (Debian/Ubuntu:
#   apt install git gcc flex bison gcc-mingw-w64-i686 gcc-mingw-w64-x86-64)
set -euo pipefail

here=$(cd "$(dirname "$0")" && pwd)
work=$(realpath -m "${1:-wine-fix-build}")
mkdir -p "$work"

if [ ! -d "$work/wine" ]; then
	git clone --depth 1 --branch wine-10.0 https://github.com/wine-mirror/wine "$work/wine"
	git -C "$work/wine" apply "$here"/patches/wine-10.0/*.patch
fi

mkdir -p "$work/build"
cd "$work/build"
if [ ! -f Makefile ]; then
	# Only the PE side of two DLLs is needed, so leave out every optional library.
	../wine/configure --enable-archs=i386,x86_64 --without-x --without-freetype \
		--without-gstreamer --without-pulse --without-alsa --without-oss --without-cups \
		--without-sane --without-gphoto --without-krb5 --without-netapi --without-opencl \
		--without-pcap --without-usb --without-v4l2 --without-vulkan --without-wayland \
		--without-gnutls --without-dbus --without-sdl --without-capi --without-ffmpeg \
		--without-osmesa --without-opengl --without-unwind --without-udev --without-fontconfig
fi
make -j"$(nproc)" __tooldeps__
make -j"$(nproc)" dlls/winmm/i386-windows/winmm.dll dlls/iccvid/i386-windows/iccvid.dll

# Strip debug info, and clear the "Wine builtin DLL" marker so Wine loads
# these from the game folder as native DLLs instead of ignoring them.
for dll in winmm iccvid; do
	i686-w64-mingw32-strip -o "$work/$dll.dll" "dlls/$dll/i386-windows/$dll.dll"
	python3 - "$work/$dll.dll" <<'EOF'
import sys
path = sys.argv[1]
data = bytearray(open(path, 'rb').read())
marker = b'Wine builtin DLL'
i = data.find(marker)
if not 0 < i < 0x80:
    sys.exit('builtin marker not found in ' + path)
data[i:i + len(marker)] = bytes(len(marker))
open(path, 'wb').write(data)
EOF
done
echo "Built: $work/winmm.dll $work/iccvid.dll"
