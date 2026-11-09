"""Verify the native query probe's offsets against the exact recovered renderer."""
import hashlib,json,struct,sys
import pefile,capstone
raw=open(sys.argv[1],'rb').read()
assert hashlib.sha256(raw).hexdigest()=='ebfb5b47cebc2d4482e912b8090bd343f717bab8a178b5a6385dec9105e7c433'
p=pefile.PE(data=raw);c=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64);c.detail=True

def ins(r):return next(c.disasm(p.get_data(r,15),r))
def rip(r):
 i=ins(r)
 m=next(x.mem for x in i.operands if x.type==capstone.x86.X86_OP_MEM and x.mem.base==capstone.x86.X86_REG_RIP)
 return i.address+i.size+m.disp
assert ins(0xa8f80).op_str=='rcx, 0x18'
assert ins(0xa8f84).op_str=='0xa81b0'
assert ins(0xa83a3).op_str=='rcx, qword ptr [r15 + 0x10]'
assert ins(0xa83c5).op_str=='r10, qword ptr [rcx]'
assert p.get_data(0xa83d9,4).hex()=='41ff5250'
assert ins(0x123ed5).op_str=='qword ptr [rcx + 0xd8]'
assert ins(0x123eeb).op_str=='qword ptr [rbp - 0x38], rcx'
assert ins(0x123ef3).op_str=='qword ptr [rbp - 0x28], rdx'
assert ins(0x123ed1).op_str=='rdx, qword ptr [r14 + 0x28]'
assert rip(0x123f2c)==0x6281e8
assert ins(0x123f33).op_str=='qword ptr [rbx], rax'
assert ins(0x123f36).op_str=='xmm0, xmmword ptr [rbp - 0x38]'
assert ins(0x123f3a).op_str=='xmmword ptr [rbx + 0x18], xmm0'
assert ins(0x123f3e).op_str=='xmm1, xmmword ptr [rbp - 0x28]'
assert ins(0x123f42).op_str=='xmmword ptr [rbx + 0x28], xmm1'
assert ins(0x123f1f).op_str=='rbx, qword ptr [rbp - 0x18]'
assert ins(0x123f46).op_str=='xmm0, xmmword ptr [rbp - 0x18]'
assert rip(0x123f4a)==0x90e9d8
assert struct.unpack('<Q',p.get_data(0x6281f0,8))[0]==p.OPTIONAL_HEADER.ImageBase+0xa8f80
points=[0x123ed5,0x123eeb,0x123ef3,0x123f1f,0x123f2c,0x123f33,0x123f36,0x123f3a,0x123f3e,0x123f42,0x123f46,0x123f4a,0xa8f80,0xa8f84,0xa83a3,0xa83c5,0xa83d9]
print(json.dumps({'status':'PASS','renderer_sha256':hashlib.sha256(raw).hexdigest(),'job_handle_rva':'0x90e9d8','job_vtable_rva':'0x6281e8','context_offset':'0x18','query_offset':'0x28','query_method_offset':'0x50','scope':'Read-only runtime target identification; no ownership or snapshot guarantee','evidence':[{'rva':hex(r),'instruction':ins(r).mnemonic+' '+ins(r).op_str} for r in points]},indent=2))
