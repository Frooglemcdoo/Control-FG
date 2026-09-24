from pathlib import Path
import sys,hashlib,json
root=Path(__file__).resolve().parents[2];here=Path(__file__).resolve().parent
sys.path.insert(0,str(root/'tools'))
from pe_tools import PE
d=PE(sys.argv[1]);want='cceb99cbd9c019af907c24c44701a53c8d230965c1fb31c325233e4c80214aa5'
assert hashlib.sha256(d.data).hexdigest()==want
assert d.exports['??1NativeTexture@d3d@@QEAA@XZ']==0x3a3e0
assert d.imports[0x5d880]=='?deleteMemory@r@@YAXPEAX@Z'
# Existing game destruction: same pointer to destructor, then engine allocator free.
checks={0x29892:'488bcb',0x29895:'e8460b0100',0x2989b:'488bcb',0x2989e:'ff15dc3f0300',
        0x3a4de:'488b8b88000000',0x3a4ed:'ff5010'}
for rva,expected in checks.items():assert d.data[d.off(rva):d.off(rva)+len(expected)//2].hex()==expected,hex(rva)
(here/'native-destroy-pair.asm').write_text(d.disasm(0x29882,0x298aa))
(here/'native-destructor.asm').write_text(d.disasm(0x3a3e0,0x3a51e))
files=[here/'audit_native.py',here/'native-destroy-pair.asm',here/'native-destructor.asm',root/'src/rr_diffuse_resize.h']
report={'status':'PASS','dll_sha256':want,'checks':{hex(k):v for k,v in checks.items()},
 'conclusion':'NativeTexture destructor at 3A3E0 then r::deleteMemory through 5D880 matches the exact game deletion pair; resource Release is inside the destructor.',
 'limits':'Static exact-DLL audit. Runtime requires creation thread, completed CPU join, native Present submission and own completed GPU fence. Windows/GPU execution not performed.',
 'tested_sha256':{p.relative_to(root).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in files}}
(here/'native-audit.json').write_text(json.dumps(report,indent=2)+'\n');print('PASS exact native destructor/free pair and resource Release instruction audit')
