"""Find the start of the function containing VA (scan back for a typical
prologue after int3/nop/ret padding) and list direct call/jmp sites to it.
usage: callers.py exe VA [VA...]"""
import sys, struct, pefile
pe = pefile.PE(sys.argv[1]); base = pe.OPTIONAL_HEADER.ImageBase
data = pe.get_memory_mapped_image()
text = [s for s in pe.sections if s.Name.startswith(b'.text')][0]
ts, te = text.VirtualAddress, text.VirtualAddress + text.Misc_VirtualSize

def func_start(rva):
    # MSVC pads functions with 0x90 (nop) or 0xcc; a function begins right after.
    i = rva
    while i > ts:
        if data[i - 1] in (0x90, 0xcc) and data[i] not in (0x90, 0xcc) and (i % 16 == 0):
            return i
        i -= 1
    return None

def callers(rva):
    out = []
    for i in range(ts, te - 5):
        op = data[i]
        if op in (0xe8, 0xe9):
            tgt = i + 5 + struct.unpack_from('<i', data, i + 1)[0]
            if tgt == rva:
                out.append(i)
    return out

for a in sys.argv[2:]:
    rva = int(a, 16) - base
    f = func_start(rva)
    print('%s: function starts at %08x' % (a, base + f))
    for c in callers(f):
        print('   %s from %08x (in function %08x)' % ('call' if data[c] == 0xe8 else 'jmp ', base + c, base + func_start(c)))
