from pathlib import Path
import hashlib,json,os,subprocess,tempfile
root=Path(__file__).resolve().parents[2];here=Path(__file__).resolve().parent
results={}
with tempfile.TemporaryDirectory() as directory:
 for mode,flags in [('normal',['-O2']),('asan_ubsan',['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer','-no-pie'])]:
  for source in ['test.cpp','observer.cpp']:
   exe=Path(directory)/(source+mode)
   subprocess.run(['g++','-std=c++17','-Wall','-Wextra','-Werror',*flags,str(here/source),'-o',str(exe)],check=True)
   results[source+':'+mode]=subprocess.check_output([str(exe)],text=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1'}).strip()
# Recheck the caller on both sides of acquiring native locks. This source gate
# supplements the executable policy/observer tests; it is not a GPU test.
runtime=(root/'src/rr_distance_runtime.h').read_text()
assert 'control_rr_reflection::Same(input.context,context)' not in runtime
assert 'control_rr_reflection::Same(context,consumerContext)' in runtime
assert runtime.count('PrimaryHandoffReason(')==2 and 'TakeOnPrimaryQueue(' in runtime
hooks=(root/'src/rr_reflection_hooks.h').read_text()
assert hooks.index('RRReflectionInstallPrimaryAppend(d3d)')<hooks.index('rrReflectionCaptureEnabled=true')
paths=[root/'src'/name for name in ['rr_reflection_capture.h','rr_reflection_present.h','rr_reflection_handoff_windows.h','rr_reflection_hooks.h','rr_distance_runtime.h','rr_user_control.h']]
paths+=list(here.glob('*.cpp'))+[here/'fixture.h',here/'run.py',here/'audit.py',here/'native-audit.json']+list(here.glob('*.asm'))
report={'status':'LOCAL_PASS_WINDOWS_GPU_REQUIRED','results':results,'windows_execution':False,'windows_seh_exercised':False,'gpu_execution':False,
 'tested_sha256':{p.relative_to(root).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}}
(here/'results.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(results,indent=2))
