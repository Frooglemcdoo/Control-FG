"""Reproduce stale Part1 evidence with a current package manifest."""
from pathlib import Path
import hashlib,json,shutil,sys,tempfile
root=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(root/'tools'))
from verify_source_evidence import verify_source_evidence
checks=[]
# Copy only files referenced by the Windows evidence gate, discovered from
# Verify-Source.ps1 itself so new evidence reports cannot silently fall outside
# this regression harness.
with tempfile.TemporaryDirectory(prefix='control-source-evidence-') as tmp:
 candidate=Path(tmp)
 import re
 verify_text=(root/'Verify-Source.ps1').read_text(encoding='utf-8-sig')
 all_refs=list(dict.fromkeys(re.findall(r"Join-Path\s+\$PSScriptRoot\s+'([^']+\.json)'",verify_text)))
 refs=[ref for ref in all_refs if ref in {'SOURCE-SHA256.json','g3-frozen-files.json'} or ref.startswith('validation/')]
 paths={'Verify-Source.ps1','SOURCE-SHA256.json','g3-frozen-files.json'}
 paths.update(refs)
 for report_path in refs:
  if report_path in {'SOURCE-SHA256.json','g3-frozen-files.json'}: continue
  report=json.loads((root/report_path).read_text(encoding='utf-8-sig'))
  paths.update((report.get('tested_sha256') or {}).keys())
 paths.update(x['Path'] for x in json.loads((root/'g3-frozen-files.json').read_text())['Files'])
 for name in paths:
  dest=candidate/name;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(root/name,dest)
 def manifest():
  data={p.relative_to(candidate).as_posix():hashlib.sha256(p.read_bytes()).hexdigest()
        for p in candidate.rglob('*') if p.is_file() and p.name!='SOURCE-SHA256.json'}
  (candidate/'SOURCE-SHA256.json').write_text(json.dumps(data))
 def reject(expected):
  try:verify_source_evidence(candidate)
  except ValueError as error:
   assert expected in str(error),str(error);return
  raise AssertionError('Stale evidence was accepted: '+expected)
 manifest();verify_source_evidence(candidate);checks.append('current source and evidence accepted')
 # Reproduce the exact r30 -> r31 failure mode: a source file advances in the
 # authoritative current-release checkpoint while an older milestone report still
 # records the previous hash. Historical evidence must not reject the new release.
 owned_source=candidate/'src/streamline_bridge.h'
 owned_original=owned_source.read_bytes()
 current_path=candidate/'validation/current-release/results.json'
 current_original=current_path.read_bytes()
 owned_source.write_bytes(owned_original+b'\n// Regression: current release advanced beyond r30 evidence.\n')
 current=json.loads(current_original)
 current['tested_sha256']['src/streamline_bridge.h']=hashlib.sha256(owned_source.read_bytes()).hexdigest()
 current_path.write_text(json.dumps(current,indent=2)+'\n')
 manifest();verify_source_evidence(candidate)
 checks.append('current-release-owned source supersedes stale historical milestone hash')
 owned_source.write_bytes(owned_original);current_path.write_bytes(current_original);manifest();verify_source_evidence(candidate)
 source=candidate/'src/rr_guide_render.h';original=source.read_bytes()
 source.write_bytes(original+b'\n// Regression: runtime source changed after Part1 tests.\n')
 manifest();reject('validation/part1/validation.json: test evidence does not match source: src/rr_guide_render.h')
 checks.append('changed guide source rejected despite refreshed package manifest')
 source.write_bytes(original)
 report_path=candidate/'validation/part1/validation.json';original_report=report_path.read_bytes()
 report=json.loads(original_report);report['tested_sha256']['src/rr_guide_render.h']='0'*64
 report_path.write_text(json.dumps(report));manifest();reject('src/rr_guide_render.h')
 checks.append('stale Part1 report rejected despite refreshed package manifest')
 report_path.write_bytes(original_report);manifest();verify_source_evidence(candidate)
 report_path.unlink();manifest()
 try:verify_source_evidence(candidate)
 except FileNotFoundError:pass
 else:raise AssertionError('Missing Part1 report was accepted')
 checks.append('missing report rejected')
files=['tools/verify_source_evidence.py','tools/validate-release.py','Verify-Source.ps1',
       'validation/part1/validation.json','validation/source-evidence/run.py']
report={'status':'PASS','checks':checks,'tested_sha256':{n:hashlib.sha256((root/n).read_bytes()).hexdigest() for n in files},
        'powershell_execution':'NOT_RUN','scope':'Actual portable packaging evidence gate, driven by Verify-Source.ps1 JSON references; no Windows execution'}
(root/'validation/source-evidence/results.json').write_text(json.dumps(report,indent=2)+'\n')
print('PASS: '+ '; '.join(checks))
