import importlib.util,struct,tempfile
from pathlib import Path
s=importlib.util.spec_from_file_location('capture',Path(__file__).with_name('analyze-instances.py'));m=importlib.util.module_from_spec(s);s.loader.exec_module(m)
def instance(x=0,mask=255):return struct.pack('<12fIIQ',1,0,0,x,0,1,0,0,0,0,1,0,7|(mask<<24),0,0x10000)
def capture(blocks):
 data=m.HEADER.pack(0x3130333933545243,1,len(blocks),20,0,0)
 for i,b in enumerate(blocks):data+=m.RECORD.pack(100+i*500,0x100000,1,2,3,len(b))+b
 return data
with tempfile.TemporaryDirectory() as d:
 p=Path(d)/'capture.bin';valid=capture([instance()*2,instance()*2,instance(2)+instance(),instance(2,0),instance(3)])
 p.write_bytes(valid);r=m.analyze(p)
 assert [x['changed_slots'] for x in r['records']]==[2,0,1,1,1]
 assert r['records'][3]['removed_tail_slots']==1 and r['records'][3]['active']==0
 assert r['records'][0]['duplicate_user_ids']==1
 for bad in [valid[:-1],valid+b'x',b'',capture([instance(float('nan'))])]:
  p.write_bytes(bad)
  try:m.analyze(p)
  except ValueError:pass
  else:raise AssertionError('Malformed capture accepted')
print('PASS: movement, masking, shrink, slot reuse, duplicate IDs, unchanged scene, truncation and invalid transform')
