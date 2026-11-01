"""Reject incomplete GI readbacks before packaging a supposedly complete RR pair."""
import copy
import importlib.util
import json
from pathlib import Path
import tempfile
spec = importlib.util.spec_from_file_location('collector', Path(__file__).with_name('collect-rr-boundary.py'))
collector = importlib.util.module_from_spec(spec)
spec.loader.exec_module(collector)
resources = []
stride = 128 * 128 * 8 * 2
for i, kind in enumerate(('material', 'normal_uvx', 'position_uvy')):
    resources.append(dict(kind=kind, slices=1, row_pitch=1024, pixel_bytes=8,
                          slice_region_bytes=128*1024, offset=i*stride))
valid = dict(schema='gi-hit-slices-r271', frame=100, gpu_fence_completed=True,
             all_batches_recorded=True, expected_batches=1, recorded_batches=1,
             rays_per_batch=1, bytes=3*stride, batches=[dict(batch=0, resources=resources)])
with tempfile.TemporaryDirectory() as tmp:
    folder=Path(tmp)
    binary=folder/'gi-hit-slices.bin'
    binary.write_bytes(bytes(valid['bytes']))
    def check(data, succeeds):
        (folder/'gi-hit-slices.json').write_text(json.dumps(data))
        try:
            collector.validate_gi_hits(folder, 100)
        except ValueError:
            if succeeds:raise
        else:
            if not succeeds:raise RuntimeError('Invalid capture accepted')
    check(valid, True)
    for key, value in [('frame',101),('gpu_fence_completed',False),('recorded_batches',0),('rays_per_batch',2),('bytes',1)]:
        bad=copy.deepcopy(valid);bad[key]=value;check(bad,False)
    bad=copy.deepcopy(valid);bad['batches'][0]['resources'][1]['offset']=0;check(bad,False)
    binary.write_bytes(bytes(valid['bytes']-1));check(valid,False)
print('PASS GI hit collector: completed data accepted; wrong frame, incomplete GPU/batches/slices, overlaps and truncated bytes rejected')
