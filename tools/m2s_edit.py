"""Edit integer variables of an .m2s save (testing aid: reach later game states).
usage: m2s_edit.py in.m2s out.m2s OBJ.VAR=VALUE ...   (numbers decimal or 0x..)
Sets existing 4-byte records; a missing one is appended. Room: 17.1=ROOM 82.53=ROOM."""
import struct, sys
sys.path.insert(0, __file__.rsplit('/', 1)[0])
import m2s
head, recs, _ = m2s.parse(sys.argv[1])
d = bytearray(open(sys.argv[1], 'rb').read())
pos = {(t, i): p for p, t, i, v in recs if len(v) == 4}
for a in sys.argv[3:]:
    k, v = a.split('=')
    o, i = (int(x, 0) for x in k.split('.'))
    v = int(v, 0)
    if (o, i) in pos:
        struct.pack_into('<i', d, pos[(o, i)] + 8, v)
    else:
        d += struct.pack('<HHIi', o, i, 4, v)
open(sys.argv[2], 'wb').write(d)
