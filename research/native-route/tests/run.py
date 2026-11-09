from pathlib import Path
import sys,re,hashlib,subprocess,json,tempfile
from pe_tools import PE
root=Path(__file__).resolve().parents[1]
p=PE(sys.argv[1])
assert hashlib.sha256(p.data).hexdigest()=='cceb99cbd9c019af907c24c44701a53c8d230965c1fb31c325233e4c80214aa5'
allowed={0x51e40,0x51ea0,0x51ef0,0x51f50,0x52010,0x528d0,0x52a20}
for a,b,name in [(0x1d320,0x1d45c,'rr-create'),(0x1d460,0x1e15b,'rr-evaluate')]:
 s=p.disasm(a,b)
 for line in s.splitlines():
  if re.search(r'\b(call|jmp)\s',line):
   m=re.search(r'\b(?:call|jmp)\s+0x([0-9a-f]+)',line);assert m,line
   target=int(m[1],16)-p.base
   assert target in allowed or a<=target<b,line
 (root/'evidence'/f'{name}.asm').write_text(s)
size=max(va+rs for _,va,rs,rp in p.sections);image=bytearray(size)
for _,va,rs,rp in p.sections:image[va:va+rs]=p.data[rp:rp+rs]
with tempfile.TemporaryDirectory() as tmp:
 raw=Path(tmp)/'image';raw.write_bytes(image);exe=Path(tmp)/'contract'
 subprocess.run(['g++','-std=c++17','-O2','-Wall','-Wextra','-Werror',str(root/'tests/native_contract.cpp'),'-o',str(exe)],check=True)
 out=subprocess.check_output([str(exe),str(raw)],text=True)
 (root/'evidence'/'native-execution.txt').write_text(out)
 print(out.splitlines()[-1])
(root/'evidence'/'result.json').write_text(json.dumps({'status':'PASS','method':'Exact uploaded DLL helper instructions executed on x86-64 Linux with Microsoft ABI NGX mocks','ngx_gpu_execution':False,'windows_execution':False,'native_dll_sha256':hashlib.sha256(p.data).hexdigest()},indent=2)+'\n')
