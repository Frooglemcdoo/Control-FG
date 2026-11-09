from pathlib import Path
import hashlib,json,os,subprocess,tempfile,sys
root=Path(__file__).resolve().parents[2];here=Path(__file__).resolve().parent
results={}
with tempfile.TemporaryDirectory() as tmp:
 source=(root/'src/rr_performance.h').read_text().replace('__try {','try {').replace('__leave;','throw PerfLeave{};').replace('__except(EXCEPTION_EXECUTE_HANDLER)','catch(const PerfLeave&){} catch(...)')
 (Path(tmp)/'performance-runtime.inc').write_text(source)
 for mode,flags in [('normal',['-O2']),('asan_ubsan',['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer','-no-pie'])]:
  for name in ['policy','adapter']:
   exe=Path(tmp)/(name+'-'+mode)
   subprocess.run(['g++','-std=c++17','-Wall','-Wextra','-Werror','-Wno-unused-function',*flags,'-I'+str(root/'src'),'-I'+tmp,str(here/(name+'.cpp')),'-o',str(exe)],check=True)
   results[name+':'+mode]=subprocess.check_output([str(exe)],text=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1'}).strip()
collector_result=subprocess.check_output([sys.executable,str(here/'collector.py')],text=True).strip()
paths=[root/'src'/n for n in ['rr_performance.h','rr_performance_policy.h','rr_evaluation_entry.h','rr_native_frame.h','rr_diffuse_replay.h','rr_diffuse_join.h','rr_reflection_runtime.h','rr_live_guides.h','rr_distance_runtime.h','probe.cpp']]+list(here.glob('*.cpp'))+[here/'run.py',here/'collector.py']+[root/n for n in ['Collect-ControlFG-Compact-Logs.ps1','Collect-ControlFG-Compact-Logs.cmd','Summarize-ControlFG-Performance.ps1']]
report={'status':'LOCAL_PASS_WINDOWS_GPU_REQUIRED','results':results,'windows_execution':False,'gpu_execution':False,'collector_powershell_7_linux':collector_result,'windows_powershell_5_1':'NOT_RUN','tested_sha256':{p.relative_to(root).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}}
(here/'results.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(results,indent=2))
