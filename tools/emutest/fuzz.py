import pefile, capstone, random, subprocess, sys, collections
from capstone import x86_const as X
from unicorn import *
from unicorn.x86_const import *
exe='/home/user/gamedata/mm2dk2001/cab/Program_files_DK/mm2run.exe'
pe=pefile.PE(exe); base=pe.OPTIONAL_HEADER.ImageBase
img=pe.get_memory_mapped_image()
md=capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32); md.detail=True; md.skipdata=True
insns={}
for i in md.disasm(img[0x1000:0xb0000], 0x401000):
    if i.id==0: continue
    if i.mnemonic in ('int3','hlt','in','out','insb','insd','outsb','outsd','iretd','retf','ljmp','lcall','sti','cli','int','int1','into','bound','les','lds','arpl','wait','fwait'): continue
    if i.mnemonic.startswith('f') : 
        pass
    insns.setdefault(bytes(i.bytes), i)
lst=list(insns.values()); random.seed(int(sys.argv[2]) if len(sys.argv)>2 else 1)
N=int(sys.argv[1]) if len(sys.argv)>1 else 3000
REGS=[UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EBX,UC_X86_REG_ESP,UC_X86_REG_EBP,UC_X86_REG_ESI,UC_X86_REG_EDI]
CSREG={X.X86_REG_EAX:0,X.X86_REG_ECX:1,X.X86_REG_EDX:2,X.X86_REG_EBX:3,X.X86_REG_ESP:4,X.X86_REG_EBP:5,X.X86_REG_ESI:6,X.X86_REG_EDI:7}
pattern=bytes(((i*7+3)&0xff) for i in range(0x10000))
def undef(m):
    if m in('and','or','xor','test'): return 0x10
    if m in('shl','sal','shr','sar','rol','ror','rcl','rcr','shld','shrd'): return 0x810|0xd4 if False else 0x810
    if m in('mul','imul'): return 0xd4
    if m in('div','idiv'): return 0xfff
    if m in('bsf','bsr'): return 0x8d5
    if m in('bt','bts','btr','btc'): return 0x8d4
    return 0
H=subprocess.Popen(['/tmp/claude-0/emutest/harness'],stdin=subprocess.PIPE,stdout=subprocess.PIPE,text=True)
bad=collections.Counter(); unimpl=collections.Counter(); ok=0; examples={}
for n in range(N):
    i=random.choice(lst)
    regs=[random.choice([random.getrandbits(32), random.randint(0,255), random.randint(0,0x40), 0xffffffff, 0x80000000, 0x7fffffff, 0]) for _ in range(8)]
    for op in i.operands:
        if op.type==X.X86_OP_MEM:
            if op.mem.base in CSREG: regs[CSREG[op.mem.base]]=0x104000+random.randint(0,0x2000)
            if op.mem.index in CSREG: regs[CSREG[op.mem.index]]=random.randint(0,0x40)
    if i.mnemonic.startswith(('rep','movs','stos','lods','scas','cmps')) or i.mnemonic in('repne scasb',):
        regs[6]=0x104000+random.randint(0,0x100); regs[7]=0x106000+random.randint(0,0x100); regs[1]=random.randint(0,20)
    regs[4]=0x10c000; 
    if random.random()<0.5: regs[5]=0x10b000
    fl=random.getrandbits(12)&0x8d5|2
    code=bytes(i.bytes)
    try:
        mu=Uc(UC_ARCH_X86,UC_MODE_32)
        mu.mem_map(0x100000,0x10000); mu.mem_write(0x100000,pattern)
        mu.mem_map(0x200000,0x1000); mu.mem_write(0x200000,code)
        for k,r in enumerate(REGS): mu.reg_write(r,regs[k])
        mu.reg_write(UC_X86_REG_EFLAGS,fl)
        mu.emu_start(0x200000,0x200000+len(code),count=0 if i.mnemonic.startswith('rep') else 1)
    except UcError as e:
        continue
    ur=[mu.reg_read(r) for r in REGS]; uf=mu.reg_read(UC_X86_REG_EFLAGS)&0xcd5; ueip=mu.reg_read(UC_X86_REG_EIP)
    mem=mu.mem_read(0x100000,0x10000)
    umem=sorted('%x=%02x'%(0x100000+k,mem[k]) for k in range(0x10000) if mem[k]!=pattern[k])
    H.stdin.write(' '.join('%x'%v for v in regs)+' %x %s\n'%(fl,code.hex())); H.stdin.flush()
    out=H.stdout.readline().split()
    key=i.mnemonic
    if out[0]=='ERR':
        if 'unmapped' in ' '.join(out): continue
        unimpl[key+' '+' '.join(out[1:4])]+=1; continue
    mr=[int(x,16) for x in out[1:9]]; mf=int(out[9],16); meip=int(out[10],16); mmem=sorted(out[11:])
    m=~undef(key)&0xcd5
    if i.mnemonic in('shl','sal','shr','sar','rol','ror','rcl','rcr','shld','shrd'):
        m &= ~0x10
    if ur!=mr or (uf&m)!=(mf&m) or ueip!=meip or umem!=mmem:
        bad[key]+=1
        if bad[key]<=2: print('MISMATCH', i.mnemonic, i.op_str, code.hex(), 'in', ['%x'%v for v in regs], 'fl %x'%fl, '\n  uc', ['%x'%v for v in ur], 'fl %x eip %x'%(uf,ueip), umem[:6], '\n  me', ['%x'%v for v in mr], 'fl %x eip %x'%(mf&0xcd5,meip), mmem[:6])
    else: ok+=1
print('ok',ok,'bad',dict(bad)); print('unimpl',unimpl.most_common(20))
