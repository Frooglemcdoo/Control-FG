"""Verify the optional shader keeps the accepted ray policy outside its center."""
import argparse,base64,hashlib,json,subprocess
from pathlib import Path
from rt_spot_projection import projection_origin
from rt_spot_authored_clip import authored_clip
from rt_spot_endpoint import BASE_SHA
root=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--dxc');args=p.parse_args()
f=root/'assets/rt-spot-projection';m=json.loads((f/'manifest.json').read_text());data=base64.b64decode((f/'rt_spot_projection.dxil.b64').read_text())
assert hashlib.sha256(data).hexdigest()==m['sha256'] and len(data)==m['bytes']
assert m['baseline_sha256']==BASE_SHA and m['extra_rays']==0
if args.dxc:
 baseline=root/'build/projection-test-baseline.dxil'
 baseline.write_bytes(base64.b64decode((root/'assets/rt-spot/rt_spot_full.dxil.b64').read_text()))
 assert hashlib.sha256(baseline.read_bytes()).hexdigest()==BASE_SHA
 source=subprocess.check_output([args.dxc,'-dumpbin',str(baseline)],text=True)
 changed,edits=projection_origin(source)
 assert len(edits)==13
 restored=changed
 for before,after in reversed(edits):restored=restored.replace(after,before)
 assert restored==source
 expected,_=authored_clip(changed)
 actual=subprocess.check_output([args.dxc,'-dumpbin',str(root/'build/rt_spot_projection.dxil')],text=True)
 # Assembly only: complete function body must retain every expected instruction.
 def body(s):return s[s.index('define void @"\\01?spotRayGeneration'):s.index('\n}',s.index('define void @"\\01?spotRayGeneration'))+2]
 normalize=lambda s:'\n'.join(line.split(';',1)[0].rstrip() for line in body(s).splitlines())
 assert normalize(actual)==normalize(expected), 'Projection raygen does not match audited transform'
 for i in range(4):
  for axis in 'xyz':assert f'%SpotSegment{i}{axis}Center = fsub fast float %Projection{axis}' in actual
 print('PASS exact signed raygen instructions, unchanged point branch/handlers/filter inputs and authored clipping')
print('PASS projection shader hash, size, source baseline and no extra rays')
