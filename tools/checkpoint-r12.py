"""Validate and package the r12 full source, without claiming Windows execution."""
from pathlib import Path
import hashlib,json,subprocess,sys,zipfile
root=Path(__file__).resolve().parents[1]
for script in ('validation/g12-current/run.py','validation/part1/run.py','validation/live-guides/run.py'):
 subprocess.run([sys.executable,str(root/script)],check=True)
if len(sys.argv)!=3: raise SystemExit('Usage: checkpoint-r12.py PATH_TO_R5_LOG_ZIP ORIGINAL_D3D_DLL')
subprocess.run([sys.executable,str(root/'validation/evaluation-entry/run.py'),sys.argv[2]],check=True)
subprocess.run([sys.executable,str(root/'validation/reflectance-gpu/run.py'),sys.argv[1]],check=True)
subprocess.run([sys.executable,str(root/'tools/audit-worker-depth.py'),sys.argv[2]],check=True)
digest=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
baseline=root.parent/'g12-r11'
frozen=json.loads((root/'g3-frozen-files.json').read_text())['Files']
for f in frozen:assert digest(root/f['Path']).upper()==f['SHA256'].upper(),f['Path']
# Stronger than the historical 11-file list: every pre-existing runtime source
# is unchanged except the declared integration/identity edits.
allowed={'src/probe.cpp','src/rr_albedo_native.h'}
unchanged=[]
if baseline.exists():
 for old in (baseline/'src').rglob('*'):
  if old.is_file():
   rel=old.relative_to(baseline).as_posix()
   if rel not in allowed:
    assert digest(root/rel)==digest(old),rel
    unchanged.append(rel)
capture=(root/'src/rr_albedo_capture.h').read_text()
assert capture.count('capture->part1Preparation=RRPart1Observe(renderer,frame,"prepare");')==1
assert capture.count('const auto part1Joined=RRPart1Observe(renderer,frame,"join");')==1
assert 'ReadEngineFrameSafe(&after,&fault)' in capture
diagnostic=(root/'src/rr_part1_diagnostic.h').read_text()
resource=(root/'src/rr_part1_resource.h').read_text()
assert all(x not in resource for x in ('->Map(', '->AddRef(', '->Release(', 'ResourceBarrier(', 'CopyBufferRegion('))
assert all(x in resource for x in ('->GetDesc()', '->GetGPUVirtualAddress()', '->GetHeapProperties(', 'sample.mappingMode!=2'))
assert 'RR_GUIDE_G12_PART1_RESOURCE' in capture
assert 'first4_then_powers_of_two' in (root/'src/probe.cpp').read_text()
assert all(x not in diagnostic for x in ('ID3D12','ResourceBarrier','CopyBufferRegion','AddRef','Release('))
part1=json.loads((root/'validation/part1/validation.json').read_text())
assert part1['status']=='PASS' and len(part1['results'])==14
for n,h in part1['tested_sha256'].items():assert digest(root/n)==h
metadata=(root/'Build-Metadata.ps1').read_text()
assert "Version = '1.0.0-RR-Native-G12-r12'" in metadata
assert 'RRPart1MetadataDiagnosticEnabled = $true' in metadata
assert 'RRPart1ResourceAcquisitionEnabled = $true' in metadata
assert 'RRPart1ReadbackEnabled = $true' in metadata
render=(root/'src/rr_guide_render.h').read_text()
readback=(root/'src/rr_part1_readback.h').read_text()
assert 'RRPart1CopyForGuide(job,capture,reason)' in capture
assert 'RRGuideRelease(part1Readback); RRGuideRelease(part1Source);' in render
assert 'control_rr_part1::CanMap(job->part1Copied,job->fence->GetCompletedValue())' in render
assert 'source->AddRef();job->part1Source=source;' in readback
assert 'RRGuideTryAcquireNativeLock(current.recordingLock)' in readback
assert 'RRGuideReleaseNativeLock(current.recordingLock)' in readback
assert 'RRPart1SameContext(again,current)' in readback
assert 'part1_source_frame' in (root/'tools/diagnostics/Collect-ControlFG-Logs.ps1').read_text()
gpu=json.loads((root/'validation/reflectance-gpu/validation.json').read_text())
assert gpu['status']=='LOCAL_PASS_WINDOWS_GPU_REQUIRED' and len(gpu['results'])==8
for n,h in gpu['tested_sha256'].items():assert digest(root/n)==h,n
build=(root/'Build.cmd').read_text()
assert 'tools\\compile-reflectance.cpp' in build and 'build\\compile-reflectance.exe' in build
assert 'RRReflectanceGpuRun(job,&view,&reflectanceOwner)' in render
assert 'gpuOkay && RRGuideExportCapture' in render
assert 'RRReflectanceGpuRetire(reflectanceOwner)' in render
host=(root/'src/rr_reflectance_gpu.h').read_text()
assert host.index('c->uncertain=true')<host.index('ExecuteCommandLists')<host.index('c->queue->Signal')<host.index('c->uncertain=false')<host.index('c->readback->Map')
assert 'CompareCandidate(source,c->mapped' in host
assert 'RRGpuReflectanceCandidateCaptureEnabled = $true' in metadata
summary={'version':'1.0.0-RR-Native-G12-r12','source_revision':'r12-rr-native-g12',
 'status':'LOCAL_SOURCE_GATES_PASS_WINDOWS_VALIDATION_REQUIRED',
 'frozen_files_verified':len(frozen),'additional_unchanged_runtime_files':unchanged,
 'reflectance':'normal and ASan/UBSan PASS','part1':'normal and ASan/UBSan PASS',
 'runtime_change':'Worker replay preserves inherited native DSV without consulting shared tracker; primary depth state checks retained; no RR evaluation',
 'gpu_reflectance':'r11 copied 580 diffuse frames before a primary-view worker depth tracker read 0xC0. r12 Windows execution pending','evaluation_entry_host_tests':'normal and ASan/UBSan PASS','live_emitter_retirement_slot_tests':'normal and ASan/UBSan PASS','windows_msvc':'NOT_RUN','powershell_parser':'NOT_RUN_ON_THIS_HOST','runtime':'NOT_RUN',
 'rr_evaluation_enabled':False,'gpu_resource_acquisition':True,'lifetime_proven':False,
 'lifetime_design':'Three live slots hold source references until successful native-queue fence completion after Present; recording/signal/device-loss uncertainty stops live generation and retains resources'}
(root/'validation/SUMMARY.json').write_text(json.dumps(summary,indent=2)+'\n')
(root/'validation/g12-current/source-contract.json').write_text(json.dumps(summary,indent=2)+'\n')
paths=[p for p in sorted(root.rglob('*')) if p.is_file() and p.relative_to(root).parts[0] not in ('build','release','third_party') and '__pycache__' not in p.parts]
manifest={p.relative_to(root).as_posix():digest(p).upper() for p in paths if p.name not in ('SOURCE-SHA256.json','ARTIFACT-SHA256.json')}
for name in ('SOURCE-SHA256.json','ARTIFACT-SHA256.json'):
 (root/name).write_text(json.dumps(manifest,indent=2)+'\n')
subprocess.run([sys.executable,str(root/'tools/validate-release.py'),str(root)],check=True)
out=root.parent/'Control-FG-v1.0.0-RR-Native-G12-r12-Full-Source.zip'
with zipfile.ZipFile(out,'w',zipfile.ZIP_DEFLATED) as z:
 for p in paths:z.write(p,p.relative_to(root).as_posix())
with zipfile.ZipFile(out) as z:
 assert z.testzip() is None
 for name,want in manifest.items():assert hashlib.sha256(z.read(name)).hexdigest().upper()==want
print('PASS: all archive manifest entries, CRC, frozen core and declared runtime diff')
print(out)
