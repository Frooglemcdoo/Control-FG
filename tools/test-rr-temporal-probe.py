import importlib.util
from pathlib import Path
p=Path(__file__).with_name('analyze-rr-temporal-probe.py')
spec=importlib.util.spec_from_file_location('probe',p);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
def log(values):
 return '\n'.join(f'rr_temporal_probe build=R192 frame={i+100} samples=4 attempt={i+1} valid=1 temporal={v} camera_cut=0 stage=pre_reflection_filter cpu_provider=1 gpu_confirmed=0 reason=ready' for i,v in enumerate(values))
r=m.analyze(log([i%8 for i in range(512)]));assert r['segments'][0]['observed_period_up_to_64']==8
r=m.analyze(log([0]*256));assert r['segments'][0]['observed_period_up_to_64']==1
r=m.analyze(log([(0xffffff80+i)&0xffffffff for i in range(256)]));assert r['segments'][0]['all_increment_by_one']
r=m.analyze(log([i%8 for i in range(255)]));assert not r['sufficient_window'] and r['segments'][0]['observed_period_up_to_64'] is None
base=log([i%8 for i in range(256)])
for key,value in [('valid=1','valid=0'),('camera_cut=0','camera_cut=1'),('samples=4','samples=16'),('gpu_confirmed=0','gpu_confirmed=1'),('reason=ready','reason=provider_changed')]:
 lines=base.splitlines();lines[128]=lines[128].replace(key,value);r=m.analyze('\n'.join(lines));assert not r['sufficient_window']
lines=base.splitlines();del lines[128];assert not m.analyze('\n'.join(lines))['sufficient_window']
assert not m.analyze('')['sufficient_window']
print('PASS: cycles, constant values, uint32 wrap, short windows, cuts, gaps, invalid reads and provenance rejection')
