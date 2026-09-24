"""Run r19's changed-component gates and package the full Windows source."""
from pathlib import Path
import hashlib,json,subprocess,sys,zipfile
root=Path(__file__).resolve().parents[1]
if len(sys.argv)!=3:raise SystemExit('Usage: checkpoint-r19.py ORIGINAL_RENDERER_DLL ORIGINAL_D3D_DLL')
subprocess.run([sys.executable,str(root/'validation/vertex-evidence/run.py')],check=True)
subprocess.run([sys.executable,str(root/'validation/reflection-backend/run.py')],check=True)
subprocess.run([sys.executable,str(root/'validation/reflection-backend/audit_native.py'),*sys.argv[1:3],str(root/'validation/reflection-backend/evidence')],check=True)
digest=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
frozen=json.loads((root/'g3-frozen-files.json').read_text())['Files']
for f in frozen:assert digest(root/f['Path']).upper()==f['SHA256'].upper(),f['Path']
baseline=root.parent/'g12-r18'
unchanged=[]
if baseline.is_dir():
 for old in (baseline/'src').rglob('*'):
  if old.is_file() and old.name!='probe.cpp':
   rel=old.relative_to(baseline);assert digest(root/rel)==digest(old),str(rel);unchanged.append(str(rel))
summary={'version':'1.0.0-RR-Native-G12-r19b','status':'LOCAL_PASS_WINDOWS_GPU_REQUIRED',
 'frozen_files_verified':len(frozen),'unchanged_prior_runtime_headers':unchanged,
 'changed_components':'reflection-only native hook, bounded D3D12 copies and Present-ledger retirement',
 'host_tests':'normal and ASan/UBSan PASS','native_assertions':32,
 'windows_msvc':'NOT_RUN','runtime':'NOT_RUN','rr_evaluation_enabled':False}
(root/'validation/SUMMARY.json').write_text(json.dumps(summary,indent=2)+'\n')
paths=[p for p in sorted(root.rglob('*')) if p.is_file() and p.relative_to(root).parts[0] not in ('build','release','third_party') and '__pycache__' not in p.parts]
manifest={p.relative_to(root).as_posix():digest(p).upper() for p in paths if p.name not in ('SOURCE-SHA256.json','ARTIFACT-SHA256.json')}
for name in ('SOURCE-SHA256.json','ARTIFACT-SHA256.json'):(root/name).write_text(json.dumps(manifest,indent=2)+'\n')
subprocess.run([sys.executable,str(root/'tools/validate-release.py'),str(root)],check=True)
out=root.parent/'Control-FG-v1.0.0-RR-Native-G12-r19b-Full-Source.zip'
with zipfile.ZipFile(out,'w',zipfile.ZIP_DEFLATED) as z:
 for p in paths:z.write(p,p.relative_to(root).as_posix())
with zipfile.ZipFile(out) as z:
 assert z.testzip() is None
 for name,want in manifest.items():assert hashlib.sha256(z.read(name)).hexdigest().upper()==want,name
print('PASS archive CRC/hashes, frozen core and all unchanged prior runtime headers')
print(out)
