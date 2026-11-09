"""The diagnostic-only collector must include only this process's GI trace."""
import importlib.util
import os
import json,struct
from pathlib import Path
import tempfile
spec=importlib.util.spec_from_file_location('collector',Path(__file__).with_name('collect-rr-boundary.py'))
c=importlib.util.module_from_spec(spec);spec.loader.exec_module(c)
with tempfile.TemporaryDirectory() as tmp:
 root=Path(tmp);local=root/'local';logs=local/'Control-FG/Diagnostics';logs.mkdir(parents=True)
 game=root/'game';game.mkdir();output=root/'output';output.mkdir()
 blob=bytes(range(256))*2
 for suffix in [*(f'frame{i:02}.bin' for i in range(1,9)),'reference.bmp']:
  (logs/f'GI-Joint-R284-123-{suffix}').write_bytes(blob)
  (logs/f'GI-Joint-R284-1234-{suffix}').write_bytes(b'other session')
 for suffix in [*(f'frame{i:02}.{ext}' for i in range(1,9) for ext in ('bin','json')),'reference.bmp']:
  (logs/f'GI-Joint-R285-123-{suffix}').write_bytes(blob)
  (logs/f'GI-Joint-R285-1234-{suffix}').write_bytes(b'other session')
 (logs/'GI-Joint-R283-123.bin').write_bytes(blob)
 (logs/'GI-Joint-R283-124.bin').write_bytes(b'other session')
 (logs/'GI-Joint-R282-123.bin').write_bytes(blob)
 (logs/'GI-Joint-R282-124.bin').write_bytes(b'other session')
 (logs/'GI-Joint-R281-123.bin').write_bytes(blob)
 (logs/'GI-Joint-R281-124.bin').write_bytes(b'other session')
 (logs/'GI-Ray-R278-123.bin').write_bytes(blob)
 (logs/'GI-Ray-R278-124.bin').write_bytes(b'other session')
 (logs/'GI-Ray-R277-123.bin').write_bytes(blob)
 (logs/'GI-Ray-R277-124.bin').write_bytes(b'other session')
 (logs/'GI-Spot-R274-123.bin').write_bytes(blob)
 (logs/'GI-Spot-R274-124.bin').write_bytes(b'other session')
 (logs/'Lighting-R204-test-123-0.log').write_text('GI_SPOT_R274_EXPORT complete=1')
 old=os.environ.get('TEMP');os.environ['TEMP']=str(root/'empty')
 try:c.collect_logs(game,local,123,output)
 finally:
  if old is None:os.environ.pop('TEMP',None)
  else:os.environ['TEMP']=old
 for suffix in [*(f'frame{i:02}.bin' for i in range(1,9)),'reference.bmp']:
  assert (output/f'logs/GI-Joint-R284-123-{suffix}').read_bytes()==blob
  assert not (output/f'logs/GI-Joint-R284-1234-{suffix}').exists()
 for suffix in [*(f'frame{i:02}.{ext}' for i in range(1,9) for ext in ('bin','json')),'reference.bmp']:
  assert (output/f'logs/GI-Joint-R285-123-{suffix}').read_bytes()==blob
  assert not (output/f'logs/GI-Joint-R285-1234-{suffix}').exists()
 assert (output/'logs/GI-Joint-R283-123.bin').read_bytes()==blob
 assert not (output/'logs/GI-Joint-R283-124.bin').exists()
 assert (output/'logs/GI-Joint-R282-123.bin').read_bytes()==blob
 assert not (output/'logs/GI-Joint-R282-124.bin').exists()
 assert (output/'logs/GI-Joint-R281-123.bin').read_bytes()==blob
 assert not (output/'logs/GI-Joint-R281-124.bin').exists()
 assert (output/'logs/GI-Ray-R278-123.bin').read_bytes()==blob
 assert not (output/'logs/GI-Ray-R278-124.bin').exists()
 assert (output/'logs/GI-Ray-R277-123.bin').read_bytes()==blob
 assert not (output/'logs/GI-Ray-R277-124.bin').exists()
 assert (output/'logs/GI-Spot-R274-123.bin').read_bytes()==blob
 assert not (output/'logs/GI-Spot-R274-124.bin').exists()
 assert (logs/'GI-Spot-R274-123.bin').read_bytes()==blob
 # Source/RR engine-frame joins must reject missing frames, overflow and mismatches.
 for i in range(1,9):
  name=logs/f'GI-Joint-R285-123-frame{i:02}'
  name.with_suffix('.bin').write_bytes(struct.pack('<4I',1,0x285,16,0)+bytes(256))
  name.with_suffix('.json').write_text(json.dumps(dict(schema='gi-stack-r285',pid=123,frame_index=i,present=100+i,engine_frame=500 if i==1 else 0,width=2560,height=1440,step=16,phase_x=0,phase_y=0,records=1,overflow=False)))
 assert c.validate_stack_sources(local,123,500)['same_frame_verified']
 try:c.validate_stack_sources(local,123,501);raise AssertionError('mismatched engine frame accepted')
 except ValueError:pass
 p=logs/'GI-Joint-R285-123-frame08.json';m=json.loads(p.read_text());m['overflow']=True;p.write_text(json.dumps(m))
 try:c.validate_stack_sources(local,123,500);raise AssertionError('overflow accepted')
 except ValueError:pass
 p.unlink()
 try:c.validate_stack_sources(local,123,500);raise AssertionError('missing frame accepted')
 except ValueError:pass
print('PASS R274 collector: exact binary bytes, PID isolation, originals retained')
