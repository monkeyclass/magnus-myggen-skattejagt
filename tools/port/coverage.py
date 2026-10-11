#!/usr/bin/env python3
"""Reports how much of each object class's handler (vtable slot 6, the
function only) the runs executed, from the engine's "coverage" output.

usage: coverage.py game.asm rooms.txt typed.c run1.cov [run2.cov ...]
game.asm: objdump -d -M intel of mm2run.exe from 0x401000 to 0x476000.
"""
import bisect
import re
import sys


def main():
    asm, rooms, typed = sys.argv[1:4]
    insns = []
    for line in open(asm):
        m = re.match(r'\s+([0-9a-f]+):\t', line)
        if m:
            insns.append(int(m.group(1), 16))
    starts = sorted(int(a, 16) for a in re.findall(r'^// ==== \S+ @ ([0-9a-f]+)', open(typed, encoding='latin-1').read(), re.M))
    covered = set()
    for f in sys.argv[4:]:
        covered.update(int(l, 16) for l in open(f) if l.strip())
    total_all = hit_all = 0
    for line in open(rooms):
        vt = re.search(r'vt=([0-9a-f]+)', line)
        s6 = re.search(r's6=([0-9a-f]+)', line)
        if not vt or not s6:
            continue
        start = int(s6.group(1), 16)
        i = bisect.bisect_right(starts, start)
        end = starts[i] if i < len(starts) else 0x476000
        lo, hi = bisect.bisect_left(insns, start), bisect.bisect_left(insns, end)
        body = insns[lo:hi]
        hit = sum(1 for a in body if a in covered)
        total_all += len(body)
        hit_all += hit
        print('%-4s vt=%s s6=%06x %5d/%-5d %3d%%' % (line.split()[0], vt.group(1), start, hit, len(body), 100 * hit // max(len(body), 1)))
    print('total %d/%d %d%%' % (hit_all, total_all, 100 * hit_all // max(total_all, 1)))


main()
