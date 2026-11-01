"""Reject incomplete GI readbacks before packaging a supposedly complete RR pair."""
import copy
import importlib.util
import json
from pathlib import Path
import tempfile
import contextlib
import io
import os
from unittest.mock import patch
spec = importlib.util.spec_from_file_location('collector', Path(__file__).with_name('collect-rr-boundary.py'))
collector = importlib.util.module_from_spec(spec)
spec.loader.exec_module(collector)
# An interrupted capture has no valid pair and may still have an open raw file.
# Recovery must package logs immediately without asking for or waiting on capture.
with tempfile.TemporaryDirectory() as tmp:
    local = Path(tmp)
    root = local / 'ControlFGProbe'
    root.mkdir()
    (root / 'probe-test-123.log').write_text('RR_INPUT_CAPTURE_C1_REQUEST accepted=1\n')
    attempt = root / 'rr-input-c1-123-100'
    attempt.mkdir()
    (attempt / 'metadata.pending').write_text('{incomplete')
    (attempt / 'color.bin').write_bytes(b'partial image')
    output = local / 'output'
    stdout = io.StringIO()
    with patch.dict(os.environ, LOCALAPPDATA=str(local)), \
         patch('sys.argv', ['collector', '--pid', '123', '--output', str(output), '--diagnostics-only', '--gi-sequence']), \
         patch.object(os, 'startfile', create=True), contextlib.redirect_stdout(stdout):
        assert collector.main() == 0
    report_path = next(output.rglob('boundary-report.json'))
    report = json.loads(report_path.read_text())
    assert report['diagnostics_only'] and not report['complete'] and report['pairs'] == []
    assert list(output.rglob('probe-test-123.log'))
    assert list(output.rglob('metadata.pending'))
    assert not list(output.rglob('color.bin'))
    assert list(output.rglob('*.zip'))
    assert 'click CAPTURE RR' not in stdout.getvalue()
    assert (attempt / 'color.bin').read_bytes() == b'partial image'
print('PASS busy recovery: packages logs and incomplete metadata without waiting, copying raw images, or deleting originals')
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
    small=copy.deepcopy(valid)
    offset=0
    for resource in small['batches'][0]['resources']:
        if resource['kind']=='material':resource.update(pixel_bytes=2,row_pitch=256,slice_region_bytes=128*256)
        resource['offset']=offset;offset+=resource['slice_region_bytes']*2
    small['bytes']=offset;binary.write_bytes(bytes(offset));check(small,True)
    binary.write_bytes(bytes(valid['bytes']))
    for key, value in [('frame',101),('gpu_fence_completed',False),('recorded_batches',0),('rays_per_batch',2),('bytes',1)]:
        bad=copy.deepcopy(valid);bad[key]=value;check(bad,False)
    bad=copy.deepcopy(valid);bad['batches'][0]['resources'][1]['offset']=0;check(bad,False)
    binary.write_bytes(bytes(valid['bytes']-1));check(valid,False)
    binary.write_bytes(bytes(valid['bytes']));check(valid,True)
    data=dict(gi_hit_capture_requested=True,frame=100,resources=[])
    (folder/'metadata.json').write_text(json.dumps(data))
    collector.copy_capture_frame(folder,data,folder/'package')
    assert {p.name for p in (folder/'package').iterdir()} == {'metadata.json','gi-hit-slices.json','gi-hit-slices.bin'}
    collector.validate_gi_hits(folder/'package',100)
print('PASS GI hit collector: completed data accepted; wrong frame, incomplete GPU/batches/slices, overlaps and truncated bytes rejected')
