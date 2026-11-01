"""The diagnostic-only collector must include only this process's GI trace."""
import importlib.util
import os
from pathlib import Path
import tempfile
spec=importlib.util.spec_from_file_location('collector',Path(__file__).with_name('collect-rr-boundary.py'))
c=importlib.util.module_from_spec(spec);spec.loader.exec_module(c)
with tempfile.TemporaryDirectory() as tmp:
 root=Path(tmp);local=root/'local';logs=local/'Control-FG/Diagnostics';logs.mkdir(parents=True)
 game=root/'game';game.mkdir();output=root/'output';output.mkdir()
 blob=bytes(range(256))*2
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
 assert (output/'logs/GI-Ray-R277-123.bin').read_bytes()==blob
 assert not (output/'logs/GI-Ray-R277-124.bin').exists()
 assert (output/'logs/GI-Spot-R274-123.bin').read_bytes()==blob
 assert not (output/'logs/GI-Spot-R274-124.bin').exists()
 assert (logs/'GI-Spot-R274-123.bin').read_bytes()==blob
print('PASS R274 collector: exact binary bytes, PID isolation, originals retained')
