"""Audit actual native GI IR, phase edit boundaries and sample/pass index coverage."""
import argparse,base64,hashlib,subprocess,tempfile
from pathlib import Path
from gi_long_sequence import ROOT,HASHES,transform,edits
p=argparse.ArgumentParser();p.add_argument('--dxc',required=True);a=p.parse_args()
with tempfile.TemporaryDirectory() as tmp:
 for variant in (0,1):
  blob=base64.b64decode((ROOT/f'research/gi-sequence-r264/gi-ray-{variant}.dxbc.b64').read_text());assert hashlib.sha256(blob).hexdigest()==HASHES[variant]
  path=Path(tmp)/'native.dxbc';path.write_bytes(blob)
  source=subprocess.check_output([a.dxc,'-dumpbin',str(path)],text=True)
  old,new,_,_=edits(variant);patched=transform(source,variant)
  assert patched.replace(new,old)==source
  for bad in (source.replace(old,old.replace('mul i32','add i32')),source.replace('i32 1112, i32 7','i32 1116, i32 7')):
   try:transform(bad,variant)
   except AssertionError:pass
   else:raise AssertionError('Unexpected source accepted')
for rays in (1,2,4,6,8,16):
 for passes in (1,2,4,8,16):
  for start in (0,1,255,256,0xffffff80):
   ids=[(((((start+f)&0xffffffff)&255)*passes+q)*rays)+i for f in range(256) for q in range(passes) for i in range(rays)]
   assert len(set(ids))==256*passes*rays and min(ids)==0 and max(ids)==256*passes*rays-1
print('PASS both exact GI libraries, reversible edit, source rejection, pass/ray separation, bounded phases and uint32 wrap; not a GPU quality test')
