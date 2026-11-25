#!/bin/sh
# usage: dart.sh HOLD_SECONDS -> throws once in both (darts running in c28), prints dart positions
export DISPLAY=:5; P=/tmp/claude-0/play/c28
xdotool keydown space; echo pkeyd:32 >> $P/ctl; sleep $1; xdotool keyup space; echo pkeyu:32 >> $P/ctl; sleep 6
n=$(date +%s%N); import -window root -crop 640x480+0+0 /tmp/claude-0/cmp/d_o.png; echo shot:9$1 | tr -d . >> $P/ctl; sleep 1
f=$P/shot9$(echo $1 | tr -d .).png
python3 - /tmp/claude-0/cmp/d_o.png $f $1 <<'PY'
import sys
from PIL import Image
def dart(f):
    im=Image.open(f).convert('RGB'); pts=[(x,y) for x in range(0,600,1) for y in range(0,400,1) if im.getpixel((x,y))[0]>200 and im.getpixel((x,y))[1]<60 and im.getpixel((x,y))[2]<60]
    # the dart's red flights: cluster away from the board's red ring is hard; report the lowest-left red blob outside the board radius test
    return None
print(sys.argv[3])
PY
cp /tmp/claude-0/cmp/d_o.png /tmp/claude-0/cmp/d_o_$1.png; cp $f /tmp/claude-0/cmp/d_n_$1.png
# next throw: a key resets the dart (state 2 -> 0)
xdotool key Left; echo pkey:276 >> $P/ctl; sleep 1
