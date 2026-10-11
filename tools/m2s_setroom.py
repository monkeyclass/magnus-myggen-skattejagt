"""Write a copy of an .m2s save with the current room changed (testing aid).
Sets records (type 17, id 1) and (type 82, id 53), which hold the room number.
usage: m2s_setroom.py in.m2s out.m2s ROOM"""
import struct, sys
sys.path.insert(0, __file__.rsplit('/', 1)[0])
import m2s
d = bytearray(open(sys.argv[1], 'rb').read())
_, recs, _ = m2s.parse(sys.argv[1])
for p, t, i, v in recs:
    if (t, i) in ((17, 1), (82, 53)):
        struct.pack_into('<i', d, p + 8, int(sys.argv[3]))
open(sys.argv[2], 'wb').write(d)
