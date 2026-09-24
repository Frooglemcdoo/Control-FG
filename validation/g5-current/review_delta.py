#!/usr/bin/env python3
"""Audit G5 runtime delta against the shipped G4 baseline."""
from pathlib import Path
import argparse, hashlib, json
p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('g5-release'));p.add_argument('--baseline',type=Path,default=Path('g4-release'));p.add_argument('--output',type=Path,default=Path('g5-validation/runtime-delta.json'));a=p.parse_args()
sha=lambda b:hashlib.sha256(b).hexdigest()
read=lambda root,name:(root/name).read_bytes()
checks=[]
def ck(name,ok):
    checks.append({'name':name,'passed':bool(ok)})
    if not ok:raise AssertionError(name)
old=read(a.baseline,'src/probe.cpp');new=read(a.source,'src/probe.cpp')
for g5,g4 in [(b'1.0.0-RR-Native-G5',b'1.0.0-RR-Native-G4'),(b'r1-rr-native-g5',b'r1-rr-native-g4'),(b'NativeG5JoinedFrameMaterialImages',b'NativeG4PrimaryViewMaterialImages')]:new=new.replace(g5,g4)
added=b'    // RR diagnostic only: preserve production AA/FG ordering, then give a\n    // joined material capture one later same-frame copy opportunity.\n    if (result) RRGuideTryCaptureJoined(call, "post_aa");\n'
ck('probe_only_identity_and_post_aa_call',new.count(added)==1 and new.replace(added,b'')==old)
old=read(a.baseline,'src/rr_albedo_capture.h');new=read(a.source,'src/rr_albedo_capture.h')
added=b'    // OriginalJoin has returned and all capture identities/counts passed. Try\n    // the existing guarded copy now; an SRV/context preflight rejection leaves\n    // the later reflection or post-AA opportunity available in this frame.\n    RRGuideTryCaptureJoined(aaCount.load(), "primary_join");\n'
ck('albedo_only_post_join_call',new.count(added)==1 and new.replace(added,b'')==old)
old=read(a.baseline,'src/rr_guide_render.h');new=read(a.source,'src/rr_guide_render.h')
start=old.index(b'static void RRGuideTryCapture(');end=old.index(b'\nstatic HRESULT RRGuideSubmit(',start)
new_start=new.index(b'// Reflection remains the preparation/fallback opportunity.');new_end=new.index(b'\nstatic HRESULT RRGuideSubmit(',new_start)
ck('guide_render_only_scheduler_delta',old[:start]==new[:new_start] and old[end:]==new[new_end:])
body=b'    RRGuideInputSnapshot input{};'
old_body=old[old.index(body,start):end]
new_body=new[new.index(body,new_start):new.index(b'\nstatic void RRGuideTryCapture(',new_start)]
ck('existing_input_and_copy_body_unchanged',old_body==new_body)
changed=[]
for f in sorted((a.source/'src').rglob('*')):
    if f.is_file() and (a.baseline/f.relative_to(a.source)).is_file() and f.read_bytes()!=(a.baseline/f.relative_to(a.source)).read_bytes():changed.append(str(f.relative_to(a.source)))
ck('only_three_runtime_files_changed',changed==['src/probe.cpp','src/rr_albedo_capture.h','src/rr_guide_render.h'])
frozen=json.loads((a.source/'g3-frozen-files.json').read_text())['Files']
ck('eleven_frozen_hashes',len(frozen)==11 and all(sha(read(a.source,x['Path']))==x['SHA256'].lower() for x in frozen))
ck('overlay_matches_g4_applied_r1',read(a.source,'src/fg_overlay.h')==read(a.baseline,'src/fg_overlay.h'))
ck('all_strict_input_and_present_checks_retained',read(a.source,'src/rr_guide_inputs.h')==read(a.baseline,'src/rr_guide_inputs.h') and old[end:]==new[new_end:])
report={'passed':all(x['passed'] for x in checks),'checks':checks,'changed_runtime_files':changed,'source_hashes':{name:sha(read(a.source,name)) for name in changed},'baseline':'Shipped G4 source; applied overlay R1','limits':['Static source comparison does not validate native resource availability, runtime performance, GPU execution or crash resolution.']}
a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,indent=2)+'\n');print('RUNTIME_DELTA=PASS CHECKS='+str(len(checks)))
