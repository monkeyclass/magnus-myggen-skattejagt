"""Convert a single-track MODE1/2352 .bin to a 2048-byte-sector .iso. usage: bin2iso.py in.bin out.iso"""
import sys
with open(sys.argv[1], 'rb') as fi, open(sys.argv[2], 'wb') as fo:
    while True:
        s = fi.read(2352)
        if len(s) < 2352:
            break
        fo.write(s[16:2064])
