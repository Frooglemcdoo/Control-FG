from pathlib import Path
import hashlib,json,os,subprocess,tempfile
root=Path(__file__).resolve().parents[2];here=Path(__file__).resolve().parent
results={}
with tempfile.TemporaryDirectory() as tmp:
 source=(root/'src/rr_diffuse_replay.h').read_text().split('#include "rr_diffuse_view.h"')[0]
 (Path(tmp)/'diffuse-prepare.inc').write_text(source)
 for mode,flags in [('normal',['-O2']),('asan_ubsan',['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer','-no-pie'])]:
  for name in ['diffuse','distance']:
   exe=Path(tmp)/(name+'-'+mode)
   subprocess.run(['g++','-std=c++17','-Wall','-Wextra','-Werror',*flags,'-I'+str(root/'src'),'-I'+tmp,str(here/(name+'.cpp')),'-o',str(exe)],check=True)
   results[name+':'+mode]=subprocess.check_output([str(exe)],text=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1'}).strip()
# Ordering/wiring gates supplement actual host code, not Windows/GPU proof.
probe=(root/'src/probe.cpp').read_text();assert probe.index('originalPresent();')<probe.index('RRDiffuseAfterPresent(count)')
native=(root/'src/rr_native_frame.h').read_text();assert native.index('RRDiffuseRequestExtent(frame,width,height)')<native.index('rrNativeResetOriginal(outputWidth',native.index('RRDiffuseRequestExtent'))
assert 'rrNativeFrame.Resizing()' in native and 'rrDiffuse->resizePending' in native
live=(root/'src/rr_live_guides.h').read_text();assert 'c->diffuseCopied=0;++c->epoch' in live
reflection=(root/'src/rr_reflection_runtime.h').read_text();assert 'c->capture.Idle()' in reflection and '++c->epoch' in reflection
distance=(root/'src/rr_distance_runtime.h').read_text();assert distance.index('RRDistanceCanResize(owner,reflection->epoch)')<distance.index('RRDistanceReleaseOutputs(owner)')
assert distance.index('owner->lastUseEpoch=reflection->epoch')<distance.index('RRDistanceRecordLeaf(owner')
paths=[root/'src'/name for name in ['probe.cpp','rr_diffuse_replay.h','rr_diffuse_resize.h','rr_diffuse_draw.h','rr_diffuse_join.h','rr_distance_runtime.h','rr_distance_resize.h','rr_live_guides.h','rr_reflection_runtime.h','rr_native_frame.h','rr_frame_coordinator.h','rr_user_control.h']]+list(here.glob('*.cpp'))+[here/'run.py']
report={'status':'LOCAL_PASS_WINDOWS_GPU_REQUIRED','results':results,'windows_execution':False,'windows_seh_exercised':False,'gpu_execution':False,
 'tested_sha256':{p.relative_to(root).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}}
(here/'results.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(results,indent=2))
