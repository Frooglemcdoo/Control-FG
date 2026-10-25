import importlib.util
import json
from pathlib import Path
import tempfile
s=importlib.util.spec_from_file_location('collector',Path(__file__).with_name('collect-rr-boundary.py'))
c=importlib.util.module_from_spec(s);s.loader.exec_module(c)
with tempfile.TemporaryDirectory() as tmp:
 root=Path(tmp)
 for i in range(2):
  folder=root/f'rr-input-c1-7-{100+i}';folder.mkdir()
  resources=[dict(file=n,width=2,height=2,pixel_bytes=8,copied=True) for n in ['input-color.rgba16f','rr-output.rgba16f']]
  for r in resources:(folder/r['file']).write_bytes(bytes(32))
  data=dict(schema='control-rr-gi-r28',pid=7,capture_ordinal=1,pair_index=i,frame=100+i,previous_frame=100 if i else 0,consecutive=bool(i),output_captured=True,rr_result=1,buffers=2,resources=resources)
  (folder/'metadata.json').write_text(json.dumps(data))
 assert len(c.read_pair(root,7,1,0))==2
 assert c.read_pair(root,8,1,0) is None and c.read_pair(root,7,2,0) is None
 assert c.read_pair(root,7,1,10**12) is None
 second=root/'rr-input-c1-7-101'/'metadata.json';data=json.loads(second.read_text())
 for field,value in [('output_captured',False),('previous_frame',99),('consecutive',False),('rr_result',0xBAD00001)]:
  changed=dict(data);changed[field]=value;second.write_text(json.dumps(changed))
  try:c.read_pair(root,7,1,0)
  except ValueError:pass
  else:raise AssertionError(field)
 second.write_text(json.dumps(data));(second.parent/'rr-output.rgba16f').write_bytes(bytes(4))
 try:c.read_pair(root,7,1,0)
 except ValueError:pass
 else:raise AssertionError('truncated image accepted')
print('PASS R151 RR collector: session/ordinal separation, fresh complete consecutive pairs, output failures and truncated images rejected')
