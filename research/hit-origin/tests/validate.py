from pathlib import Path
import os,subprocess,sys,tempfile,json,hashlib
root=Path(__file__).resolve().parents[1];out=root/'evidence';results={}
dxc=Path(sys.argv[1]).resolve()
with tempfile.TemporaryDirectory() as folder:
 for mode,flags in [('normal',[]),('sanitized',['-fsanitize=address,undefined','-fno-omit-frame-pointer','-no-pie'])]:
  exe=Path(folder)/mode
  subprocess.run(['g++','-std=c++17','-Wall','-Wextra','-Werror',*flags,str(root/'tests/test_origin.cpp'),'-o',str(exe)],check=True)
  env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1'}
  results[mode]=subprocess.check_output([str(exe),str(out/'native-origin-fixtures.txt')],env=env,text=True).strip()
 subprocess.run([str(dxc),'-T','cs_6_0','-E','main','-Ges','-WX','-Fo',str(out/'hit-distance-from-depth.dxil'),'-Fc',str(out/'hit-distance-from-depth.ll'),str(root/'src/rr_hit_distance_from_depth.hlsl')],check=True)
 results['HLSL']='PASS cs_6_0 compile'
text=(out/'hit-distance-from-depth.ll').read_text()
for name,offset in [('WorldFromViewRow0',0),('Width',48),('ClipToViewColumn0',64),('ClipToViewColumn3',112),('NativeInvOutputRes',128)]:
 import re
 assert re.search(name+r';\s*; Offset:\s*'+str(offset)+r'\b',text),name
results['constant_layout']='PASS 144-byte cbuffer offsets'
(out/'validation.json').write_text(json.dumps({'results':results,'source_sha256':{p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in (root/'src').iterdir()},'runtime_wired':False,'gpu_executed':False,'rr_enabled':False},indent=2)+'\n')
print(json.dumps(results,indent=2))
