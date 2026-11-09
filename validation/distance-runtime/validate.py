from pathlib import Path
import hashlib,json,os,re,subprocess,sys,tempfile
root=Path(__file__).resolve().parents[2];here=Path(__file__).resolve().parent;dxc=Path(sys.argv[1]).resolve()
digest=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
pairs=[('src/shaders/rr_hit_distance_shared.h','research/hit-distance/src/rr_hit_distance_shared.h'),('src/shaders/rr_hit_origin_shared.h','research/hit-origin/src/rr_hit_origin_shared.h'),('src/rr_provider_auth.h','research/provider-auth/src/rr_provider_auth.h')]
for a,b in pairs:assert digest(root/a)==digest(root/b),(a,b)
results={};sources=['validation/distance-runtime/input.cpp','research/hit-distance/tests/test_distance.cpp','research/hit-origin/tests/test_origin.cpp','research/provider-auth/tests/test.cpp']
with tempfile.TemporaryDirectory() as tmp:
 for mode,flags in [('normal',['-O2']),('asan_ubsan',['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer','-no-pie'])]:
  for rel in sources:
   source=root/rel;exe=Path(tmp)/(source.stem+mode)
   subprocess.run(['g++','-std=c++17','-Wall','-Wextra','-Werror',*flags,str(source),'-o',str(exe)],check=True)
   args=[str(exe)]+([str(root/'research/hit-origin/evidence/native-origin-fixtures.txt')] if source.stem=='test_origin' else [])
   results[rel+':'+mode]=subprocess.check_output(args,text=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1'}).strip()
 ir=Path(tmp)/'distance.ll'
 subprocess.run([str(dxc),'-T','cs_6_0','-E','main','-Ges','-WX','-Fo',str(Path(tmp)/'distance.dxil'),'-Fc',str(ir),str(root/'src/shaders/rr_distance.hlsl')],check=True)
 text=ir.read_text()
 for name,offset in [('WorldFromViewRow0',0),('Width',48),('ClipToViewColumn0',64),('ClipToViewColumn3',112),('NativeInvOutputRes',128)]:assert re.search(name+r'.*Offset:\s*'+str(offset)+r'\b',text),name
 assert 'NumThreads=(8,8,1)' in text
 (here/'shader.json').write_text(json.dumps({'status':'PASS','compiler':'Linux DXC cs_6_0, strict, warnings as errors','dxc_sha256':digest(dxc),'root_constants':36,'offsets_verified':[0,48,64,112,128],'threads':[8,8,1],'windows_fxc_cs_5_0':'NOT_RUN','gpu_execution':False,'sha256':{rel:digest(root/rel) for rel in ['src/shaders/rr_distance.hlsl','src/shaders/rr_hit_distance_shared.h','src/shaders/rr_hit_origin_shared.h','src/rr_distance_input.h','tools/compile-distance.cpp']}},indent=2)+'\n')
(here/'math.json').write_text(json.dumps({'status':'PASS','results':results,'production_headers_byte_identical_to_tested_headers':pairs,'sha256':{a:digest(root/a) for a,b in pairs+[("validation/distance-runtime/input.cpp",""),("src/rr_distance_input.h","")]},'gpu_execution':False},indent=2)+'\n')
print('PASS distance/origin/provider math normal + ASan/UBSan; production header identity; distance HLSL and reflected constants')
