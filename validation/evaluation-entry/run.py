from pathlib import Path
import hashlib,json,os,subprocess,sys,tempfile
root=Path(__file__).resolve().parents[2];here=Path(__file__).resolve().parent
if len(sys.argv)>1:
 subprocess.run([sys.executable,str(root/'tools/audit-evaluation.py'),sys.argv[1]],check=True)
else:
 audit=json.loads((root/'validation/evaluation-boundary.json').read_text())
 assert audit['status']=='STATIC_EXACT_BINARY_PASS' and audit['d3d_sha256']=='cceb99cbd9c019af907c24c44701a53c8d230965c1fb31c325233e4c80214aa5'
 print('Retaining prior exact-binary audit; rerunning changed gateway host tests.')
results={}
with tempfile.TemporaryDirectory() as tmp:
 for mode,flags in [('normal',['-O2']),('asan_ubsan',['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer','-no-pie'])]:
  for name in ['tail-test','entry-test']:
   exe=Path(tmp)/(name+'-'+mode)
   subprocess.run(['g++','-std=c++17','-Wall','-Wextra','-Werror',*flags,str(here/(name+'.cpp')),'-o',str(exe)],check=True)
   results[name+'-'+mode]=subprocess.check_output([str(exe)],text=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1'}).strip()
report={'status':'LOCAL_PASS_WINDOWS_REQUIRED','results':results,'windows_hook_execution':'NOT_RUN','rr_evaluation_enabled':True,'limitations':'Host Win32 mocks do not validate Windows SEH, page permissions, installation timing, or game execution.',
 'tested_sha256':{str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest() for p in [root/'src/rr_evaluation_entry.h',root/'src/rr_evaluation_tail.h',here/'entry-test.cpp',here/'tail-test.cpp']}}
(here/'results.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
