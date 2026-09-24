from pathlib import Path
import hashlib,json,os,re,subprocess,sys,tempfile
root=Path(__file__).resolve().parents[2];here=Path(__file__).resolve().parent
sys.path.insert(0,str(root/'tools'))
from pe_tools import PE
p=PE(sys.argv[1]);assert hashlib.sha256(p.data).hexdigest()=='ebfb5b47cebc2d4482e912b8090bd343f717bab8a178b5a6385dec9105e7c433'
checks=[]
def check(name,okay):
 assert okay,name
 checks.append(name)
def code(name,rva,hexbytes):
 b=bytes.fromhex(hexbytes);check(name,p.data[p.off(rva):p.off(rva)+len(b)]==b)
code('RR selects GI history release and jumps over filter',0x12cd3e,'48 8d 0d 2b 78 7e 00 e8 d6 6f ee ff 84 c0 74 11 48 8d 0d 73 1d 7e 00 e8 96 70 ee ff e9 ec 01 00 00')
code('shared history release detaches before pool release',0x13dfd,'48 8b 11 48 c7 01 00 00 00 00 48 85 d2 74 0d')
regions=[
 (0x12bd9d,0x12bdc7,'gi-radius',['0x180914570','test   al,al','jne','mulss  xmm0,xmm10']),
 (0x150d4,0x151e9,'gi-history-recreate',['cmp    QWORD PTR [rsi],0x0','0x1800a9760','0x1805dfd40']),
 (0x12b9b6,0x12b9e8,'transparent-producer',['0x250','0x1800a5390']),
 (0x12dbe3,0x12dc47,'gi-composite-source',['0x18090e9f8','0x1805dff90']),
 (0x137c9d,0x137cc2,'debris-menu',['0x6f0','0x18090e9d0']),
]
for a,b,name,tokens in regions:
 s=p.disasm(a,b)
 for token in tokens:check(name+':'+token,token in s)
 (here/(name+'.asm')).write_text(s)
options=[(0x5ce0,0x912d00,'rt:Enable Reflections'),(0x5d30,0x912df0,'rt:Enable indirect diffuse'),
 (0x5e20,0x913060,'rt:Enable Spot Contact Shadows'),(0x5e70,0x913150,'rt:Enable Sun Shadows'),
 (0x5f60,0x9133c0,'rt:Enable AO'),(0x6500,0x914260,'rt:Enable Raytraced Transparents'),(0x3cf0,0x90e9d0,'rt:Enable mesh particles')]
for start,value,label in options:
 s=p.disasm(start,start+0x40);check(label+':value',hex(p.base+value) in s)
 strings=[]
 for match in re.finditer(r'# 0x([0-9a-f]+)',s):
  try:strings.append(p.string(int(match[1],16)-p.base))
  except (ValueError,UnicodeDecodeError):pass
 check(label+':name',label in strings)
results={}
with tempfile.TemporaryDirectory() as tmp:
 d=Path(tmp)
 native=(root/'src/rr_native_frame.h').read_text()
 observer=native[native.index('static void RRNativeHookGIRelease'):native.index('static bool RRNativeReady')]
 (d/'gi-observer.inc').write_text(observer)
 for mode,flags in [('normal',['-O2']),('asan_ubsan',['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer','-no-pie'])]:
  for name in ['policy','hook']:
   exe=d/(name+'-'+mode)
   subprocess.run(['g++','-std=c++17','-Wall','-Wextra','-Werror',*flags,'-I'+tmp,str(here/(name+'.cpp')),'-o',str(exe)],check=True)
   results[name+':'+mode]=subprocess.check_output([str(exe)],text=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1'}).strip()
 size=max(va+rs for _,va,rs,rp in p.sections);image=bytearray(size)
 for _,va,rs,rp in p.sections:image[va:va+rs]=p.data[rp:rp+rs]
 (d/'image').write_bytes(image);exe=d/'native'
 subprocess.run(['g++','-std=c++17','-O2','-Wall','-Wextra','-Werror',str(here/'native.cpp'),'-o',str(exe)],check=True)
 results['native']=subprocess.check_output([str(exe),str(d/'image')],text=True).strip()
paths=[root/'src'/n for n in ['rr_rt_stack.h','rr_native_frame.h','rr_frame_coordinator.h','rr_evaluation_entry.h','rr_user_control.h']]+list(here.glob('*.cpp'))+[here/'run.py']
report={'status':'LOCAL_PASS_WINDOWS_GPU_REQUIRED','checks':checks,'results':results,'renderer_sha256':hashlib.sha256(p.data).hexdigest(),
 'scope':'Full game RT menu compatibility; native GI bypass; existing shadow/transparency/debris paths retained',
 'limits':['Native pool release and GPU calls mocked','Windows hooks and full-stack GPU image quality not run','Opaque reflection guide does not describe secondary transparent surfaces'],
 'tested_sha256':{v.relative_to(root).as_posix():hashlib.sha256(v.read_bytes()).hexdigest() for v in paths}}
(here/'results.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({'checks':len(checks),'results':results},indent=2))
