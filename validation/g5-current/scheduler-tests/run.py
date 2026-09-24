#!/usr/bin/env python3
"""Compile and run actual G5 scheduler and complete albedo capture header."""
import argparse,hashlib,json,os,pathlib,re,subprocess
p=argparse.ArgumentParser()
p.add_argument('--candidate',default='/workspace/scratch/36fa93863a31/g5-release')
p.add_argument('--sanitize',action='store_true')
a=p.parse_args();root=pathlib.Path(__file__).resolve().parent;src=pathlib.Path(a.candidate)/'src'
def sha(b):return hashlib.sha256(b).hexdigest()
capture=(src/'rr_albedo_capture.h').read_text()
includes=re.findall(r'^#include "([^"]+)"$',capture,re.M)
assert includes==['rr_albedo_prepare.h','rr_albedo_draw_abi.h','rr_albedo_shader.h','rr_albedo_native.h']
(root/'capture_under_test.inc').write_text(re.sub(r'^#include "[^"]+"\n','',capture,flags=re.M))
guide=(src/'rr_guide_render.h').read_text()
start=guide.index('static void RRGuideTryCaptureAtBoundary(')
end=guide.index('\nstatic HRESULT RRGuideSubmit(',start)
stop_start=guide.index('static void RRGuideStop(')
stop_end=guide.index('\nstatic void RRGuideArm(',stop_start)
scheduler=guide[stop_start:stop_end]+'\n\n'+guide[start:end]
(root/'scheduler_under_test.inc').write_text(scheduler)
probe=(src/'probe.cpp').read_text()
aa_start=probe.index('static bool HookAA(')
aa_end=probe.index('\n// RR Phase 2 observation-only runtime hooks.',aa_start)
(root/'aa_under_test.inc').write_text(probe[aa_start:aa_end])
hashes={n:sha((src/n).read_bytes()) for n in ['rr_albedo_draw_abi.h','rr_albedo_capture.h','rr_albedo_prepare.h','rr_guide_render.h','probe.cpp']}
hashes['aa_under_test.inc']=sha((root/'aa_under_test.inc').read_bytes())
hashes['scheduler_under_test.inc']=sha(scheduler.encode())
hashes['capture_under_test.inc']=sha((root/'capture_under_test.inc').read_bytes())
(root/'tested-source-sha256.json').write_text(json.dumps(hashes,indent=2)+'\n')
binary=root/('scheduler_tests_sanitized' if a.sanitize else 'scheduler_tests')
cmd=['g++','-std=c++17','-O1','-g','-Wall','-Wextra','-Wno-unused-function','-I'+str(src),str(root/'fixtures.cpp'),'-o',str(binary)]
if a.sanitize:cmd+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
build=subprocess.run(cmd,capture_output=True,text=True)
(root/('build-sanitized.log' if a.sanitize else 'build.log')).write_text(build.stdout+build.stderr)
if build.returncode:print(build.stdout+build.stderr);raise SystemExit(build.returncode)
env=os.environ.copy()
if a.sanitize:env.update(ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
run=subprocess.run([str(binary)],capture_output=True,text=True,env=env)
(root/('results-sanitized.txt' if a.sanitize else 'results.txt')).write_text(run.stdout+run.stderr)
print(run.stdout+run.stderr)
raise SystemExit(run.returncode)
