"""Linear disassembly of a VA range. usage: dis.py exe start end (hex VAs)
Annotates pushes/movs of addresses that point at strings and IAT imports."""
import sys, pefile, capstone, re
pe = pefile.PE(sys.argv[1]); base = pe.OPTIONAL_HEADER.ImageBase
data = pe.get_memory_mapped_image()
imp = {}
for e in getattr(pe, 'DIRECTORY_ENTRY_IMPORT', []):
    for f in e.imports:
        imp[f.address] = '%s!%s' % (e.dll.decode(), (f.name or b'#%d' % f.ordinal).decode())
s, e = int(sys.argv[2], 16), int(sys.argv[3], 16)
md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
def note(op):
    out = []
    for h in re.findall(r'0x[0-9a-f]{6,8}', op):
        v = int(h, 16)
        if v in imp: out.append(imp[v])
        elif base <= v < base + len(data):
            r = data[v - base:v - base + 64].split(b'\0')[0]
            if len(r) >= 3 and all(32 <= c < 127 for c in r): out.append(repr(r.decode()))
    return ('   ; ' + ' '.join(out)) if out else ''
for ins in md.disasm(data[s - base:e - base], s):
    print('%08x  %-6s %s%s' % (ins.address, ins.mnemonic, ins.op_str, note(ins.op_str)))
