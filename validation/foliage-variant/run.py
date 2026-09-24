from pathlib import Path
import hashlib,json,os,subprocess,tempfile
root=Path(__file__).resolve().parents[2];here=Path(__file__).resolve().parent
results={}
with tempfile.TemporaryDirectory() as temp:
 for mode,flags in [('normal',['-O2']),('asan_ubsan',['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer','-no-pie'])]:
  exe=Path(temp)/mode
  subprocess.run(['g++','-std=c++17','-Wall','-Wextra',*flags,str(here/'selection-test.cpp'),'-o',str(exe)],check=True)
  subprocess.run([str(exe)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1'})
  results[mode]='PASS: actual family selection, observed foliage keys, preserving existing bit, other families and draw range policy'
audit=json.loads((here/'evidence/material-rejection-audit.json').read_text());e=audit['rejected_vertex_evidence']
assert audit['engine_frame']==4051 and len(e['pairs'])==1 and len(e['batches'])==27
assert all(b['family_kind']==4 and b['pair_index']==0 and b['keys_readable'] for b in e['batches'])
expected={'original_vs':'a954d6e56122c5c0008775c37371d1bf240c6349c89c3397c455df655cdc1ea1','replacement_vs':'6264375bdb79511a34d647af2ceb46387d601aae72f3dd05ee16ef9492b16d55','replacement_ps':'cd3f9865a91912865a87dbae62a8e41f8a8a7e9281e6f123e90106e6aae73b0b'}
for k,h in expected.items():
 blob=(here/('evidence/pair-000-'+k+'.dxbc')).read_bytes()
 assert hashlib.sha256(blob).hexdigest()==h and blob==bytes.fromhex(e['pairs'][0][k])
files=[root/'src/rr_albedo_prepare.h',here/'selection-test.cpp',here/'run.py']+list((here/'evidence').glob('*'))
report={'status':'LOCAL_PASS_WINDOWS_REQUIRED','results':results,'snapshot_frame':4051,'vertex_rejected_batches':27,'unique_pairs':1,'original_and_replacement_equal':False,'alternate_variant_runtime_match':'NOT_OBSERVED','rr_ready':False,'sha256':{p.relative_to(root).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in files}}
(here/'results.json').write_text(json.dumps(report,indent=2)+'\n');print('PASS: foliage selection and captured pair identity; alternate variant still requires runtime byte match')
