"""Read Ivanoff MM2 .idx/.dat resource packs.
mm2.idx:     u32 count, then {u32 id, u32 offset, u32 size} records
mm2lng.idx:  {u32 id, u32 offset, u32 size} records, no count (language packs)
The language packs' ids start at the shared pack's count (FUN_00480a60).
usage: mm2pack.py idx dat [list | dump ID out | hexdump ID [n]]"""
import struct, sys

def load(idx_path, dat_path):
    raw = open(idx_path, 'rb').read()
    # Only the shared pack (mm2.idx) has the count; a language pack's first
    # record has id >= 0x100 and offset 0.
    header = 4 if len(raw) % 12 == 4 else 0
    n = (len(raw) - header) // 12
    ents = {}
    for k in range(n):
        rid, off, size = struct.unpack_from('<III', raw, header + 12 * k)
        ents[rid] = (off, size)
    return ents, open(dat_path, 'rb')

if __name__ == '__main__':
    ents, dat = load(sys.argv[1], sys.argv[2])
    cmd = sys.argv[3] if len(sys.argv) > 3 else 'list'
    if cmd == 'list':
        for rid, (off, size) in sorted(ents.items()):
            dat.seek(off); h = dat.read(8)
            print('%5d %#06x off=%9d size=%8d %s %r' % (rid, rid, off, size, h.hex(), h))
    elif cmd == 'dump':
        off, size = ents[int(sys.argv[4], 0)]; dat.seek(off)
        open(sys.argv[5], 'wb').write(dat.read(size))
    elif cmd == 'hexdump':
        off, size = ents[int(sys.argv[4], 0)]; dat.seek(off)
        n = int(sys.argv[5], 0) if len(sys.argv) > 5 else 256
        d = dat.read(min(size, n))
        for i in range(0, len(d), 16):
            c = d[i:i + 16]
            print('%06x  %-48s %s' % (i, c.hex(' '), ''.join(chr(b) if 32 <= b < 127 else '.' for b in c)))
