"""Find code references to strings in a PE and disassemble around them.
usage: xref.py exe "string" [before] [after]"""
import sys, struct, pefile, capstone
pe = pefile.PE(sys.argv[1]); base = pe.OPTIONAL_HEADER.ImageBase
data = pe.get_memory_mapped_image()
needle = sys.argv[2].encode()
before = int(sys.argv[3]) if len(sys.argv) > 3 else 40
after = int(sys.argv[4]) if len(sys.argv) > 4 else 60
text = [s for s in pe.sections if s.Name.startswith(b'.text')][0]
ts, te = text.VirtualAddress, text.VirtualAddress + text.Misc_VirtualSize
md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
i = data.find(needle + b'\0')
while i != -1:
    va = base + i
    print('== string %r at %08x' % (needle, va))
    ref = struct.pack('<I', va); j = data.find(ref, ts, te)
    while j != -1:
        print('-- ref at %08x' % (base + j))
        start = max(ts, j - before)
        for ins in md.disasm(data[start:j + after], base + start):
            print('  %08x  %-8s %s' % (ins.address, ins.mnemonic, ins.op_str))
        j = data.find(ref, j + 1, te)
    i = data.find(needle + b'\0', i + 1)
