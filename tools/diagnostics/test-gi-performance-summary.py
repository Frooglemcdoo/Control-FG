import importlib.util,json,tempfile,zipfile
from pathlib import Path
spec=importlib.util.spec_from_file_location('summary',Path(__file__).with_name('summarize-gi-performance.py'));m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
with tempfile.TemporaryDirectory() as tmp:
 root=Path(tmp);run=root/'run';logs=root/'logs';send=root/'send';run.mkdir();logs.mkdir()
 marks=[dict(phase=p,qpc=q,frequency=1000) for p,q in [('STILL_BEGIN',1000),('STILL_END',2000),('PAN_BEGIN',2000),('PAN_END',3000)]]
 (run/'phases.jsonl').write_text('\n'.join(map(json.dumps,marks)))
 (run/'huge.etl').write_bytes(b'EXCLUDED'*10000)
 (logs/'probe-42.log').write_text('qpc=2000 GI_FRAME_TIMES start_qpc=1000 end_qpc=2000 frequency=1000 count=50 ticks='+','.join(['20']*50)+'\nqpc=3000 GI_FRAME_TIMES start_qpc=2000 end_qpc=3000 frequency=1000 count=25 ticks='+','.join(['40']*25)+'\nqpc=1500 GI_OWNED_CPU stage=scene_mapping calls=2 total_ms=6 max_ms=4\n')
 r=m.summarize(run,logs,42,send)
 assert r['phases']['STILL']['fps']==50 and r['phases']['PAN']['fps']==25
 assert r['phases']['PAN']['p99_ms']==40 and r['phases']['STILL']['coverage_fraction']==1
 assert r['cpu_stages'][0]['mean_ms']==3
 with zipfile.ZipFile(next(send.glob('*.zip'))) as z: assert 'huge.etl' not in z.namelist() and z.testzip() is None
 (logs/'probe-42.log').write_text('no samples\n')
 r=m.summarize(run,logs,42,send);assert 'fps' not in r['phases']['STILL']
print('PASS compact report: exact phase frames, FPS/percentiles, nested CPU mean, trace exclusion, absent samples never invent FPS')
