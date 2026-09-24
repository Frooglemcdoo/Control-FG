from pathlib import Path
import hashlib,json,re,subprocess,sys,tempfile
from pe_tools import PE
here=Path(__file__).resolve().parent
p=PE(sys.argv[1]);assert hashlib.sha256(p.data).hexdigest()=='ebfb5b47cebc2d4482e912b8090bd343f717bab8a178b5a6385dec9105e7c433'
s=p.disasm(0x13e20,0x13e7a)
for line in s.splitlines():
 if re.search(r'\bcall\b',line):assert re.search(r'call\s+0x1800a9e00',line),line
(here/'reset.asm').write_text(s)
for a,b,name,tokens in [(0x16010,0x16057,'resize-path',['0x180013e20','0x1800161c9']),
 (0x16187,0x161c9,'initial-clear',['0x180912b18','0x180912b20','0x1805dfd40']),
 (0x11b668,0x11b6c3,'native-feature-reset',['0x180914620','0x1805df758','0x180802a3f']),
 (0x11de27,0x11de45,'rr-jitter-period',['0x400','0x180914620','cmovne ecx,eax'])]:
 text=p.disasm(a,b)
 for token in tokens:assert token in text,(name,token)
 (here/(name+'.asm')).write_text(text)
size=max(va+rs for _,va,rs,rp in p.sections);image=bytearray(size)
for _,va,rs,rp in p.sections:image[va:va+rs]=p.data[rp:rp+rs]
with tempfile.TemporaryDirectory() as tmp:
 path=Path(tmp)/'image';path.write_bytes(image);exe=Path(tmp)/'test'
 subprocess.run(['g++','-std=c++17','-O2','-Wall','-Wextra','-Werror',str(here/'test.cpp'),'-o',str(exe)],check=True)
 out=subprocess.check_output([str(exe),str(path)],text=True)
(here/'results.json').write_text(json.dumps({'status':'PASS','native_instruction_test':out.strip(),'pool_release':'mocked','native_allocation_clear':'static audit only','windows_gpu_validation':False},indent=2)+'\n')
print(out.strip())
