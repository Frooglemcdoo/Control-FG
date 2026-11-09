#!/usr/bin/env python3
"""Stage current G5 evidence and produce a lean, self-checked source archive."""
from pathlib import Path
import argparse, hashlib, json, shutil, zipfile
ROOT=Path(__file__).resolve().parents[1];SOURCE=ROOT/'g5-release';CHECKS=ROOT/'g5-validation';DEST=SOURCE/'validation/g5-current'
p=argparse.ArgumentParser();p.add_argument('--package',action='store_true');a=p.parse_args()
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def copytree_filtered(base,dest):
    for path in base.rglob('*'):
        if not path.is_file() or path.suffix not in {'.md','.json','.ps1','.py','.cpp','.h','.inc','.log','.txt'}:continue
        target=dest/path.relative_to(base);target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(path,target)
DEST.mkdir(parents=True,exist_ok=True)
copytree_filtered(CHECKS,DEST)
copytree_filtered(ROOT/'g4-analysis',DEST/'g4-analysis')
provenance=DEST/'g4-evidence';provenance.mkdir(exist_ok=True)
for relative in ['package/Build.log','package/build__build-validation.json','collection-report.json']:
    path=ROOT/'g4-logs'/relative;target=provenance/Path(relative).name;shutil.copyfile(path,target)
cap=ROOT/'g4-logs/rr-guides/rr-guide-g3-20260913-144531-841-48084'
shutil.copyfile(cap/'metadata.json',provenance/'capture-metadata.json')
# Raw images are retained in the supplied evidence ZIP; no 122 MB duplication.
upload=ROOT/'upload/Control-FG-v1.0.0-RR-Native-G4-Logs-20260913-094641-dc83d4af.zip'
summary={'build':'1.0.0-RR-Native-G5','current_evidence_directory':'validation/g5-current',
 'historical_evidence':'Other validation directories refer to earlier builds, not fresh G5 runs.',
 'g4_input':{'name':upload.name,'bytes':upload.stat().st_size,'sha256':sha(upload)},
 'g4_result':{'material_frame':1754,'base_export_frame':1755,'replayed_batches':468,'ranges':4,'skipped_ranges':0,'native_material_images_exported':0,'replay_wall_ms':236.024,'last_periodic_aa_calls':4992,'last_periodic_aa_failures':0,'prior_ui_crash_resolved':False},
 'fresh_g5_checks':{'runtime_delta_checks':8,'unchanged_frozen_files':11,'actual_code_host_cases_normal':80,'actual_code_host_cases_asan_ubsan':80,'shared_contract_cases':79,'windows_target_object_compilations':4},
 'limits':['G5 has not been built with MSVC or linked on this host.','G5 has not been executed in Windows or on a GPU.','Host tests mock engine, D3D, locking, SEH and submission dependencies.','No UI-crash-resolution or every-frame performance claim.','RR evaluation, NGX guide writes and denoiser bypass remain disabled.']}
if (CHECKS/'source-validation.json').exists():summary['source_and_release_validation_passed']=read(CHECKS/'source-validation.json')['passed']
(DEST/'SUMMARY.json').write_text(json.dumps(summary,indent=2)+'\n')
manifest={}
for path in sorted(SOURCE.rglob('*')):
    if not path.is_file():continue
    rel=str(path.relative_to(SOURCE))
    if path.name in {'SOURCE-SHA256.json','ARTIFACT-SHA256.json'} or rel.startswith('validation/g5-current/'):continue
    manifest[rel]=sha(path)
(SOURCE/'SOURCE-SHA256.json').write_text(json.dumps(manifest,indent=2)+'\n')
if not a.package:print('G5 source manifest prepared:',len(manifest));raise SystemExit(0)
assert read(CHECKS/'source-validation.json')['passed']
assert read(CHECKS/'runtime-delta.json')['passed']
compile_report=read(CHECKS/'compile/compile-report.json')
assert compile_report['passed'] and len(compile_report['checks'])==4 and all(x['passed'] for x in compile_report['checks'])
assert read(CHECKS/'release-contract-fixtures.json')['cases']==79
suite=read(CHECKS/'scheduler-tests/validation-summary.json')
assert all(x['passed']==80 and x['failed']==0 for x in suite['runs'].values())
for name,h in suite['tested_sources'].items():
    src=SOURCE/'src'/name
    assert sha(src if src.is_file() else CHECKS/'scheduler-tests'/name)==h
for entry in read(SOURCE/'g3-frozen-files.json')['Files']:assert sha(SOURCE/entry['Path'])==entry['SHA256'].lower()
files={str(p.relative_to(SOURCE)):p for p in SOURCE.rglob('*') if p.is_file() and p.name!='ARTIFACT-SHA256.json'}
assert not any(p.suffix.lower() in {'.dll','.exe','.obj','.lib','.pdb','.dmp'} or p.name in {'scheduler_tests','scheduler_tests_sanitized'} for p in files.values())
all_hashes={name:{'bytes':path.stat().st_size,'sha256':sha(path)} for name,path in sorted(files.items())}
artifact=SOURCE/'ARTIFACT-SHA256.json';artifact.write_text(json.dumps(all_hashes,indent=2)+'\n');files[artifact.name]=artifact
out=ROOT/'deliverables/Control-FG-v1.0.0-RR-Native-G5-FastBuild-Full-Source.zip';out.parent.mkdir(exist_ok=True)
with zipfile.ZipFile(out,'w',zipfile.ZIP_DEFLATED,compresslevel=9) as z:
    for name,path in sorted(files.items()):z.write(path,name)
with zipfile.ZipFile(out) as z:
    assert z.testzip() is None
    for name,entry in all_hashes.items():
        data=z.read(name);assert len(data)==entry['bytes'] and hashlib.sha256(data).hexdigest()==entry['sha256']
result={'file':str(out),'bytes':out.stat().st_size,'sha256':sha(out),'members':len(files),'source_manifest_entries':len(manifest)}
(ROOT/'deliverables/g5-deliverable.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
