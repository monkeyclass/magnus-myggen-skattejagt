"""Parse MM2 .m2s save files (layout inferred).
Layout: u32 ?, u32 ?, u8[4] ?, then records {u16 type, u16 id, u32 len, u8 data[len]}.
usage: m2s.py file [file2]   (two files: print records that differ)"""
import struct, sys

def parse(path):
    d = open(path, 'rb').read()
    head, p, recs = d[:12], 12, []
    while p + 8 <= len(d):
        t, i, n = struct.unpack_from('<HHI', d, p)
        recs.append((p, t, i, d[p + 8:p + 8 + n]))
        p += 8 + n
    return head, recs, p == len(d)

def show(v):
    return str(struct.unpack('<i', v)[0]) if len(v) == 4 else v.hex()

if __name__ == '__main__':
    h1, r1, ok1 = parse(sys.argv[1])
    print('header', h1.hex(), 'records', len(r1), 'clean parse', ok1)
    if len(sys.argv) == 2:
        for p, t, i, v in r1:
            print('%5d  type %3d id %5d  %s' % (p, t, i, show(v)))
    else:
        h2, r2, ok2 = parse(sys.argv[2])
        print('header', h2.hex(), 'records', len(r2), 'clean parse', ok2)
        a = {(t, i): v for _, t, i, v in r1}; b = {(t, i): v for _, t, i, v in r2}
        for k in sorted(set(a) | set(b)):
            if a.get(k) != b.get(k):
                print('type %3d id %5d : %s -> %s' % (k[0], k[1], show(a[k]) if k in a else '-', show(b[k]) if k in b else '-'))
