#!/usr/bin/env python3
import hashlib,json,pathlib,re
p=pathlib.Path(__file__).resolve().parent
runs={}
for name in ['results.txt','results-sanitized.txt']:
    data=(p/name).read_text()
    found=re.search(r'^RESULT passed=(\d+) failed=(\d+)$',data,re.M)
    assert found and found.groups()==('80','0'),name
    assert len(re.findall(r'^PASS ',data,re.M))==80,name
    runs[name]={'passed':80,'failed':0,'sha256':hashlib.sha256(data.encode()).hexdigest()}
report={'suite':'actual G5 scheduler + complete albedo capture + complete HookAA','prior_identity_cases':48,'new_scheduling_cases':32,'runs':runs,'tested_sources':json.loads((p/'tested-source-sha256.json').read_text()),'host_limits':'Native engine/D3D, FG submissions, SRW locking, thread creation and SEH are mocked. No Windows, GPU, concurrency, or production ownership/lifetime validation.'}
(p/'validation-summary.json').write_text(json.dumps(report,indent=2)+'\n')
print('G5_SCHEDULER_TESTS=PASS cases=80 normal=PASS ASAN_UBSAN=PASS')
