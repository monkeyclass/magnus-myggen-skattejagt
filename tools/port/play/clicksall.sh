#!/bin/sh
for r in 22 23 24 25 26 27 28 29 30 31 32 34 35 36 37 38 39 40 41 42 43 44 45 49 50 51 52 53 54 55 56 57 58 59 62; do
  /tmp/claude-0/cmp/room.sh $r >/dev/null 2>&1
  echo "$r idle: $(python3 /tmp/claude-0/cmp/cmp.py /tmp/claude-0/cmp/$r 2>&1)"
  /tmp/claude-0/cmp/clicks.sh $r > /tmp/claude-0/cmp/$r/clicks.txt 2>&1
  python3 - $r <<'PY'
import sys
from PIL import Image, ImageChops
r=sys.argv[1]; d='/tmp/claude-0/cmp/'+r
lines=open(d+'/clicks.txt').read().split('\n')
for l in lines:
    p=l.split()
    if len(p)<3: continue
    i=p[0]
    try: a=Image.open('%s/co%s.png'%(d,i)).convert('RGB'); b=Image.open('%s/cn%s.png'%(d,i)).convert('RGB')
    except Exception: print(r,l,'missing'); continue
    h=ImageChops.difference(a.crop((0,0,640,415)),b.crop((0,0,640,415))).convert('L').point(lambda v:255 if v>40 else 0)
    n=h.histogram()[255]
    print(r, l, n, '%.1f%%'%(100.0*n/(640*415)), h.getbbox())
PY
done
