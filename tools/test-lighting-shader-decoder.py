import hashlib
import importlib.util
from pathlib import Path
spec=importlib.util.spec_from_file_location('decoder',Path(__file__).with_name('decode-lighting-shaders.py'))
d=importlib.util.module_from_spec(spec);spec.loader.exec_module(d)
blob=b'DXBC'+b'\x01'*300
sha=hashlib.sha256(blob).hexdigest()
lines=['shader_snapshot_begin export=1 count=1 dropped=0 capacity=8388608',f'shader_snapshot_entry export=1 index=0 stage=1 bytes={len(blob)} sha={sha}']
for offset in range(0,len(blob),128):lines.append(f'shader_snapshot_data export=1 index=0 offset={offset} hex={blob[offset:offset+128].hex()}')
lines.append('shader_snapshot_end export=1 count=1');text='\n'.join(lines)
assert d.decode(text)[0][0]['data']==blob
for bad in [text.rsplit('\n',1)[0],text.replace('offset=128','offset=0'),text.replace(sha,'0'*64),text.replace('stage=1','stage=9'),text.replace('count=1','count=2'),text+'\nshader_snapshot_begin export=2 count=1 dropped=0 capacity=8388608']:
 try:d.decode(bad)
 except ValueError:pass
 else:raise AssertionError('Invalid snapshot accepted')
assert d.decode(text.replace('dropped=0','dropped=3'))[1]==3
print('PASS R148 shader decoder: exact bytes, integrity, partial exports, duplicate/missing chunks and bounds')
