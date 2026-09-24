from pathlib import Path
import sys,subprocess,os,tempfile,hashlib,json,re
from pe_tools import PE
here=Path(__file__).resolve().parent
r=PE(sys.argv[1]);d=PE(sys.argv[2])
assert hashlib.sha256(r.data).hexdigest()=='ebfb5b47cebc2d4482e912b8090bd343f717bab8a178b5a6385dec9105e7c433'
assert hashlib.sha256(d.data).hexdigest()=='cceb99cbd9c019af907c24c44701a53c8d230965c1fb31c325233e4c80214aa5'
checks=[('target',0x164bc,0x16526,['mov    rcx,rbx','0x18128f518']),('source',0x1659c,0x165fd,['mov    rcx,rsi','0x18128f540']),('temporal-bind',0x166f2,0x16749,['mov    rcx,rdx','call   0x1801db750']),('after-temporal',0x16786,0x16805,['0x1805dfcc0','mov    r12,rsi','mov    r14,rbx','0x1805dfd48','0x180016b56']),('brdf-result',0x16ee2,0x16f65,['0x1805dfcc0','mov    r14,r12','cmp    r14,rsi','0x1805dfd48'])]
for name,a,b,need in checks:
 text=r.disasm(a,b)
 for token in need:assert token in text,(name,token)
 (here/(name+'.asm')).write_text(text)
# The filter creates source mips before binding its temporal inputs.
text=r.disasm(0x1641a,0x1643d)
assert '0x1805dfcb0' in text and '0x1801e6b40' in text
(here/'mip-preparation.asm').write_text(text)
text=r.disasm(0x1e6b88,0x1e6bdc)
assert '0x1805dfd30' in text and 'cmp    eax,0x1' in text
(here/'mip-source.asm').write_text(text)
text=d.disasm(0x3d190,0x3d612)
# Whitelist every external call executed by the native harness.
for line in text.splitlines():
 if re.search(r'\bcall\s',line):
  assert any(token in line for token in ['0x18003b000','0x180057070','0x18005d1b0','0x18005d1b8','0x18005d1a8','[rax+0x50]','[r10+0x80]']),line
(here/'native-copy.asm').write_text(text)
copy='?copyRegion@NativeTextureUtil@d3d@@SAXPEAVNativeTexture@2@HHAEBV?$Vector3Template@H@m@@0HH1PEBV45@@Z'
assert r.imports[0x5dfd48]==copy and d.exports[copy]==0x3d190
shader_root=here.parents[1]/'research/native-integration-audit'
shader=json.loads((shader_root/'evidence/deferredlight_filtering_specular-000.json').read_text())
assert hashlib.sha256((shader_root/'shaders/deferredlight_filtering_specular-000.dxbc').read_bytes()).hexdigest()==shader['sha256']
uavs=[b for b in shader['bindings'] if b['kind'] not in (0,2,3)]
assert len(uavs)==1 and uavs[0]['kind']==4 and uavs[0]['name']=='g_DLF_rwtColorTarget'
assert len([i for i in shader['instructions'] if i['name']=='store_uav_typed'])==1
results={}
with tempfile.TemporaryDirectory() as tmp:
 for name,flags in [('normal',['-O2']),('asan_ubsan',['-O1','-fsanitize=address,undefined','-fno-omit-frame-pointer','-no-pie'])]:
  exe=Path(tmp)/name
  subprocess.run(['g++','-std=c++17','-Wall','-Wextra','-Werror',*flags,str(here/'test.cpp'),'-o',str(exe)],check=True)
  results[name]=subprocess.check_output([str(exe)],text=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1'}).strip()
 size=max(va+rs for _,va,rs,rp in d.sections);image=bytearray(size)
 for _,va,rs,rp in d.sections:image[va:va+rs]=d.data[rp:rp+rs]
 path=Path(tmp)/'image';path.write_bytes(image);exe=Path(tmp)/'native-copy'
 subprocess.run(['g++','-std=c++17','-O2','-Wall','-Wextra','-Werror',str(here/'native-copy.cpp'),'-o',str(exe)],check=True)
 results['exact_native_copy']=subprocess.check_output([str(exe),str(path)],text=True).strip()
root=here.parents[1]
paths=[root/'src'/name for name in ['rr_specular_noisy.h','rr_specular_noisy_windows.h','rr_native_frame.h']]+list(here.glob('*.cpp'))+list(here.glob('*.asm'))+[here/'run.py']
(here/'results.json').write_text(json.dumps({'status':'LOCAL_PASS','native_audit_groups':len(checks)+3,'tests':results,'windows_compilation':False,'gpu_execution':False,'hook_installed':False,
 'tested_sha256':{p.relative_to(root).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}},indent=2)+'\n')
print(json.dumps(results,indent=2))
