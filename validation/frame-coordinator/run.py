from pathlib import Path
import hashlib,json,os,subprocess,tempfile
root=Path(__file__).resolve().parents[2];here=Path(__file__).resolve().parent
results={}
with tempfile.TemporaryDirectory() as tmp:
 for mode,flags in [('normal',['-O2']),('asan_ubsan',['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer','-no-pie'])]:
  for source in [here/'test.cpp',here/'recovery-test.cpp',here/'resolution-test.cpp',here/'parameters.cpp',root/'validation/distance-runtime/record.cpp']:
   exe=Path(tmp)/(source.stem+'-'+mode)
   subprocess.run(['g++','-std=c++17','-Wall','-Wextra','-Werror',*flags,str(source),'-o',str(exe)],check=True)
   results[source.relative_to(root).as_posix()+':'+mode]=subprocess.check_output([str(exe)],text=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1'}).strip()
 exe=Path(tmp)/'relay';subprocess.run(['g++','-std=c++17','-O2','-Wall','-Wextra','-Werror',str(here/'relay.cpp'),'-o',str(exe)],check=True)
 results['native-call-relay']=subprocess.check_output([str(exe)],text=True).strip()
paths=[root/'src'/name for name in ['rr_user_control.h','rr_frame_coordinator.h','rr_guide_parameters.h','rr_indirect_call.h','rr_distance_record.h']]+list(here.glob('*.cpp'))+[root/'validation/distance-runtime/record.cpp']
report={'status':'LOCAL_PASS_WINDOWS_GPU_REQUIRED','results':results,'windows_execution':False,'gpu_execution':False,'tested_sha256':{p.relative_to(root).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}}
(here/'results.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
