"""Read Ivanoff MM2 .idx/.dat resource packs.
mm2.idx:    u32 count, then count * {u32 id, u32 offset, u32 size}
mm2xx.idx:  u32 count, then {u32 offset, u32 size, u32 id} (language packs)
usage: mm2pack.py idx dat [list | dump ID out | hexdump ID [n]]"""
import struct, sys

def load(idx_path, dat_path):
    raw = open(idx_path, 'rb').read()
    n = (len(raw) - 4) // 12
    a = [struct.unpack_from('<III', raw, 4 + 12 * k) for k in range(n)]
    # Language packs store {offset,size,id}; detect by first entry offset 0.
    lang = a[0][0] == 0 and a[0][1] != 0 and a[0][2] > 0x100
    ents = {}
    for x in a:
        rid, off, size = (x[2], x[0], x[1]) if lang else x
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
