from pathlib import Path
import hashlib,json,os,subprocess,tempfile,sys
root=Path(__file__).resolve().parents[2];here=Path(__file__).resolve().parent
results={}
with tempfile.TemporaryDirectory() as temp:
 for mode,flags in [('normal',['-O2']),('asan_ubsan',['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer','-no-pie'])]:
  exe=Path(temp)/mode
  subprocess.run(['g++','-std=c++17','-Wall','-Wextra','-Werror','-D_WIN32','-I',str(here/'shim'),*flags,str(here/'test.cpp'),'-o',str(exe)],check=True)
  lines=subprocess.check_output([str(exe)],text=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1'}).splitlines()
  captured,bounded,capped=map(json.loads,lines)
  assert captured['raw_bytes']==96 and len(captured['pairs'])==1 and len(captured['batches'])==2
  for key,last in [('original_vs',1),('replacement_vs',2),('replacement_ps',3)]:
   want=bytearray(32);want[:4]=b'DXBC';want[24]=32;want[31]=last
   assert bytes.fromhex(captured['pairs'][0][key])==want
  assert [x['pair_index'] for x in captured['batches']]==[0,0]
  assert bounded['raw_bytes']==1572864 and bounded['omitted_pair_attempts']==1
  assert bounded['omitted_batch_references']==1 and len(bounded['batches'])==256
  assert all(x['pair_index'] is None for x in bounded['batches'])
  assert len(capped['pairs'])==32 and capped['omitted_pair_attempts']==2
  assert len(lines[1])<8*1024*1024
  results[mode]='PASS: actual Session capture, strict rejection, opt-in, cache, owned bytes, JSON, dedup, byte/pair/reference caps'
  export=Path(temp)/(mode+'-export');folder=Path(temp)/(mode+'-output');folder.mkdir()
  subprocess.run(['g++','-std=c++17','-Wall','-Wextra','-Werror',*flags,'-I',str(root/'validation/part1/shim'),str(here/'export-test.cpp'),'-o',str(export)],check=True)
  subprocess.run([str(export)],check=True,stdout=subprocess.DEVNULL,env={**os.environ,'CONTROL_EXPORT_TEST_ROOT':str(folder),'ASAN_OPTIONS':'detect_leaks=0:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1'})
  audit,=folder.rglob('material-rejection-audit.json')
  metadata=json.loads((audit.parent/'metadata.json').read_text());assert metadata['material_rejection_audit_bytes']==audit.stat().st_size
  extracted=Path(temp)/(mode+'-extracted')
  subprocess.run([sys.executable,str(root/'tools/extract-vertex-evidence.py'),str(audit),str(extracted)],check=True)
  for name,value in [('original_vs',1),('replacement_vs',2),('replacement_ps',3)]:
   assert (extracted/('pair-000-'+name+'.dxbc')).read_bytes()==bytes([value])*131072
  # Reuse must fail instead of overwriting an earlier inspection.
  assert subprocess.run([sys.executable,str(root/'tools/extract-vertex-evidence.py'),str(audit),str(extracted)],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL).returncode!=0
  results[mode]+='; actual exporter >256KiB audit, metadata lengths, byte-exact extractor, overwrite rejection'

files=['tools/extract-vertex-evidence.py','validation/vertex-evidence/export-test.cpp','src/rr_vertex_evidence.h','src/rr_albedo_shader.h','src/rr_albedo_capture.h','src/rr_guide_export.h','Build-Metadata.ps1','validation/vertex-evidence/test.cpp','validation/vertex-evidence/run.py','validation/vertex-evidence/shim/windows.h']
(here/'results.json').write_text(json.dumps({'status':'LOCAL_PASS_WINDOWS_REQUIRED','results':results,'sha256':{n:hashlib.sha256((root/n).read_bytes()).hexdigest() for n in files},'limits':'Native descriptor reads use valid synthetic objects and a C++ exception shim, not Windows SEH or a GPU.'},indent=2)+'\n')
print(json.dumps(results,indent=2))
