#!/usr/bin/env python3
"""usage: cmp.py ROOMDIR -> compares o*.png (original) with n*.png (ours): static difference of the first
frames, and the animated areas (pixels that change across the shots) of each; writes side.png."""
import sys, glob
from PIL import Image, ImageChops
d = sys.argv[1]
O = [Image.open(f).convert('RGB') for f in sorted(glob.glob(d + '/o*.png'))]
N = [Image.open(f).convert('RGB') for f in sorted(glob.glob(d + '/n*.png'))]
if not O or not N:
    sys.exit('missing shots')
area = (0, 0, 640, 415)  # without the subtitle bar
def animated(seq):
    m = Image.new('L', (640, 480), 0)
    for a, b in zip(seq, seq[1:]):
        m = ImageChops.lighter(m, ImageChops.difference(a, b).convert('L').point(lambda v: 255 if v > 24 else 0))
    return m.crop(area)
def boxes(m):
    # coarse 32x32 cells that animate
    cells = set()
    px = m.load()
    for cy in range(0, m.height, 32):
        for cx in range(0, m.width, 32):
            n = sum(1 for y in range(cy, min(cy + 32, m.height), 2) for x in range(cx, min(cx + 32, m.width), 2) if px[x, y])
            if n > 6:
                cells.add((cx, cy))
    return cells
# best static match: min difference over pairs (animations may be out of phase)
def diff(a, b):
    h = ImageChops.difference(a.crop(area), b.crop(area)).convert('L').point(lambda v: 255 if v > 40 else 0)
    return sum(h.histogram()[255:]) , h
best = min((diff(a, b)[0], i, j) for i, a in enumerate(O[:4]) for j, b in enumerate(N[:4]))
cnt, h = diff(O[best[1]], N[best[2]])
ao, an = boxes(animated(O)), boxes(animated(N))
print('static diff px %d (%.2f%%), animated cells: original %d, ours %d, only original %s, only ours %s' % (
    cnt, 100.0 * cnt / (640 * 415), len(ao), len(an), sorted(ao - an)[:12], sorted(an - ao)[:12]))
side = Image.new('RGB', (1280, 480 + 415))
side.paste(O[best[1]], (0, 0)); side.paste(N[best[2]], (640, 0))
side.paste(h.convert('RGB'), (0, 480)); side.paste(Image.merge('RGB', (animated(O), animated(N), Image.new('L', (640, 415)))), (640, 480))
side.save(d + '/side.png')
