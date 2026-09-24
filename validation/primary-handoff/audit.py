from pathlib import Path
import sys,hashlib,json,zipfile,re
here=Path(__file__).resolve().parent;root=here.parents[1]
sys.path.insert(0,str(root/'validation/native-history'))
from pe_tools import PE
p=PE(sys.argv[1]);assert hashlib.sha256(p.data).hexdigest()=='cceb99cbd9c019af907c24c44701a53c8d230965c1fb31c325233e4c80214aa5'
checks={
 'primary_thread_gate':(0x32874,0x32886,['0x18005d5d8','0x1800f6c34','0x180032b70']),
 'detach_and_close':(0x4300b,0x4306b,['0x180111c18','0x180043710','[rsi+0x18],rax','[rax+0x48]','[rbx+0x28],0x1']),
 'enqueue_before_replacement':(0x32962,0x329e3,['[rsi+0x18]','0x1800024e0','[rdx],0x0','[rsi+0x28]','0x1800434a0','0x180042cc0']),
 'new_primary_context':(0x42d01,0x42d3e,['0x1800f6c34','0x180001e90','0x180111c18']),
 'fifo_queue_to_d3d':(0x434e9,0x4355d,['[r13+0x30]','[r13+0x28]','[r15+rbx*8]','[rax+0x18]','inc    rbx','[rax+0x50]']),
 'append_preserves_order':(0x25e7,0x2620,['[rdx]','[r15],rax','[rdi+0x8]']),
}
for name,(a,b,need) in checks.items():
 s=p.disasm(a,b)
 for token in need:assert token in s,(name,token)
 (here/(name+'.asm')).write_text(s)
assert p.data[p.off(0x3298d):p.off(0x3298d)+9]==bytes.fromhex('488d4e28e84afbfcff')
report={'status':'PASS','checks':list(checks),'observed_hook_site':'d3d+32991 -> 24E0; Direct queue+28 after Close',
 'ordering':'primary Close -> append -> replacement context; queue array traversed in insertion order',
 'gpu_dependency':'existing copy-destination to NON_PIXEL_SHADER_RESOURCE barriers order copied inputs before their later SRV use',
 'scope':'Static exact-DLL proof plus runtime enqueue observation required; no GPU execution claimed',
 'docs':'https://learn.microsoft.com/en-us/windows/win32/direct3d12/executing-and-synchronizing-command-lists'}
(here/'native-audit.json').write_text(json.dumps(report,indent=2)+'\n');print('PASS primary close/enqueue/replacement and FIFO native audit')
