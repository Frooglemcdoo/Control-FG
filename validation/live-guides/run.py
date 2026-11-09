from pathlib import Path
import hashlib,json,os,subprocess,tempfile
root=Path(__file__).resolve().parents[2];here=Path(__file__).resolve().parent
results={}
with tempfile.TemporaryDirectory() as tmp:
 source=(root/'src/rr_guide_render.h').read_text();start=source.index('    if (ready) {',source.index('static void RRGuideTryCaptureAtBoundary('))
 end=source.index('    // Acquiring references',start)
 (Path(tmp)/'high-resolution.inc').write_text(source[start:end])
 for mode,flags in [('normal',['-O2']),('asan_ubsan',['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer','-no-pie'])]:
  for name in ['record-test','retire-test','slots-test','diffuse-test','join-test','draw-test','depth-test','frame-input-test','jitter-test','capture-test','high-resolution-test']:
   exe=Path(tmp)/(name+'-'+mode)
   subprocess.run(['g++','-std=c++17','-Wall','-Wextra','-Werror',*flags,'-I'+str(root/'src'),'-I'+tmp,str(here/(name+'.cpp')),'-o',str(exe)],check=True)
   results[name+'-'+mode]=subprocess.check_output([str(exe)],text=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1'}).strip()
paths=[root/'src/rr_albedo_depth.h',root/'src/rr_albedo_native.h']+list((root/'src').glob('rr_live*.h'))+list((root/'src').glob('rr_diffuse*.h'))+[root/'src/shaders/rr_jitter_ray.hlsli',root/'src/shaders/rr_diffuse_bits.hlsli',root/'src/rr_frame_slots.h',root/'src/shaders/rr_live_guides.hlsl',root/'tools/compile-live.cpp']+list(here.glob('*.cpp'))
paths+=[root/'src/rr_guide_render.h',root/'src/rr_guide_inputs.h',root/'src/rr_memory_budget.h',root/'src/rr_dimensions.h']
report={'status':'LOCAL_PASS_WINDOWS_GPU_REQUIRED','results':results,'shader_compile':'NOT_RUN_LOCAL_MANDATORY_WINDOWS_BUILD_GATE','gpu_execution':'NOT_RUN','rr_evaluation':False,'tested_sha256':{p.relative_to(root).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in paths},'scope':'Actual command emitter and retirement orchestration under host mocks; portable frame-slot policy. Full Windows host/preflight and shader execution are not validated by these tests.'}
(here/'results.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
