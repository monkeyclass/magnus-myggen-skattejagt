#!/bin/sh
# usage: keys.sh ROOMDIR TAG WAIT key... (xdotool key names: space Up Down Left Right Escape) -> side image
P=$1; tag=$2; w=$3; shift 3; export DISPLAY=:5
for k in "$@"; do
  case $k in space) c=32;; Up) c=273;; Down) c=274;; Right) c=275;; Left) c=276;; Escape) c=27;; Return) c=13;; *) c=0;; esac
  xdotool key $k; echo pkey:$c >> /tmp/claude-0/play/$P/ctl; sleep 0.6
done
sleep $(echo "$w/1000" | bc -l)
import -window root -crop 640x480+0+0 /tmp/claude-0/cmp/k_$tag.o.png; echo shot:7$tag >> /tmp/claude-0/play/$P/ctl; sleep 0.8
python3 -c "
from PIL import Image
o=Image.new('RGB',(1280,480)); o.paste(Image.open('/tmp/claude-0/cmp/k_$tag.o.png'),(0,0)); o.paste(Image.open('/tmp/claude-0/play/$P/shot7$tag.png'),(640,0)); o.save('/tmp/claude-0/cmp/k_$tag.png')"
echo /tmp/claude-0/cmp/k_$tag.png
