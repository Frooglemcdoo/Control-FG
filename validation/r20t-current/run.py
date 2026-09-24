from pathlib import Path
import hashlib,json,os,subprocess,tempfile
root=Path(__file__).resolve().parents[2]; here=Path(__file__).resolve().parent
results={}
with tempfile.TemporaryDirectory() as tmp:
 for mode,flags in [('normal',['-O2']),('asan_ubsan',['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer','-no-pie'])]:
  exe=Path(tmp)/('mode-'+mode)
  subprocess.run(['g++','-std=c++17','-Wall','-Wextra','-Werror',*flags,'-I'+str(root/'src'),str(here/'mode-test.cpp'),'-o',str(exe)],check=True)
  results[mode]=subprocess.check_output([str(exe)],text=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1'}).strip()
# Current-source contracts for the performance-isolation delta.
checks={
 'evaluation_full_gate': all(x in (root/'src/rr_evaluation_entry.h').read_text() for x in ['const bool fullWork=','if(fullWork) RRLiveBeforeEvaluation','fullWork?RRDistanceBeforeEvaluation']),
 'reflection_full_gate': 'RRUserRuntimeWorkRequested()' in (root/'src/rr_reflection_hooks.h').read_text() and 'RRUserRuntimeWorkRequested()' in (root/'src/rr_reflection_handoff_windows.h').read_text(),
 'diffuse_full_gate': 'if(!control_rr::RRUserRuntimeWorkRequested()) return;' in (root/'src/rr_diffuse_replay.h').read_text(),
 'runtime_fp16_guides': (root/'src/rr_live_guides.h').read_text().count('DXGI_FORMAT_R16G16B16A16_FLOAT') >= 3 and 'normal_rgba16f,specular_rgba16f,diffuse_rgba16f' in (root/'src/rr_live_guides.h').read_text(),
 'diagnostic_readbacks_disabled': 'diagnostic_readback=0' in (root/'src/rr_live_guides.h').read_text() and 'diagnostic_readback=0' in (root/'src/rr_distance_runtime.h').read_text(),
 'packed_normal_roughness': 'normalRoughnessMode=sl::DLSSDNormalRoughnessMode::ePacked' in (root/'src/rr_options_r20p.h').read_text(),
 'sparse_profiler': 'sample_every=60' in (root/'src/rr_performance.h').read_text() and '%60' in (root/'src/rr_performance.h').read_text(),
 'skin_experiment_blocked': 'r20t_performance_focus' in (root/'src/fg_overlay.h').read_text(),
 'periodic_diagnostics_off': 'const bool periodicCandidate = false' in (root/'src/probe.cpp').read_text() and 'const bool periodicTrace = false' in (root/'src/probe.cpp').read_text(),
 'observer_hooks_disabled': 'RR_OBSERVER_HOOKS_DISABLED reason=r20t_performance_cleanup' in (root/'src/probe.cpp').read_text(),
 'live_ef_preserved': all(x in ((root/'src/rr_native_preset.h').read_text()+(root/'src/rr_evaluation_entry.h').read_text()) for x in ['RR_PRESET_LIVE_CREATE','RR_PRESET_LIVE_SWITCH']),
}
if not all(checks.values()):
 raise SystemExit('FAIL source checks: '+','.join(k for k,v in checks.items() if not v))
paths=[here/'mode-test.cpp',root/'src/rr_user_control.h',root/'src/rr_evaluation_entry.h',root/'src/rr_reflection_hooks.h',root/'src/rr_reflection_handoff_windows.h',root/'src/rr_diffuse_replay.h',root/'src/rr_live_guides.h',root/'src/rr_distance_runtime.h',root/'src/rr_options_r20p.h',root/'src/rr_performance.h',root/'src/fg_overlay.h',root/'src/probe.cpp']
report={'status':'LOCAL_PASS_WINDOWS_GPU_REQUIRED','version':'1.0.0-RR-Native-G12-r20t','results':results,'checks':checks,'tested_sha256':{p.relative_to(root).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in paths},'limitations':['Portable policy/source test only; Windows/MSVC and GPU performance/resolution transitions require user validation']}
(here/'results.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({'results':results,'checks':checks},indent=2))
