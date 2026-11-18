"""Audit pinned native and owned GI hit-writer SSA; does not certify live bindings."""
import argparse,base64,hashlib,json,re,subprocess,tempfile
from pathlib import Path
from gi_single_producer import shared_hits
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--dxc',required=True);p.add_argument('--output',required=True);a=p.parse_args()
results=[]
for variant in (0,1):
 data=base64.b64decode((ROOT/f'research/gi-sequence-r264/gi-ray-{variant}.dxbc.b64').read_bytes())
 with tempfile.TemporaryDirectory() as td:
  f=Path(td)/'native.dxil';f.write_bytes(data)
  src=subprocess.run([a.dxc,'-dumpbin',str(f)],check=True,capture_output=True,text=True).stdout
 owned=shared_hits(src)
 def closest(s):
  return re.search(r'^define void [^\n]*closestHitMain[^\n]*\n.*?^}',s,re.M|re.S).group()
 # The exact writer transformation is guarded by replace_once in shared_hits.
 c=closest(owned)
 assert '%105 = select i1 %GIOwnedHit, float %RayTCurrent, float %GINativeDistance' in c
 assert '%GIOwnedHit = icmp slt i32 %GITaggedSlice, 0' in c
 assert '%GIRegularSlice = and i32 %GITaggedSlice, 2147483647' in c
 # Follow position publication back to ray endpoint, not the interpolated normal.
 for component,root in [('123','%.i0144'),('125','%.i1145'),('127','%.i2146')]:
  assert f'%{component} = fadd fast float ' in c
  assert root+' = fadd fast float ' in c
 assert re.search(r'textureStore.f32.*float %123, float %125, float %127, float %tmp157',c)
 for row in range(2,6):assert re.search(r'%sys_constants, i32 '+str(row)+r'\)',c)
 for axis in ['%.i0141','%.i1142','%.i2143']:
  assert re.search(re.escape(axis)+r' = fmul fast float %105, %WorldRayDirection',c)
 # Ensure owned selector only changes distance and slice handling; material,
 # UV, matrix rows and texture stores remain those of the native hit writer.
 native_stores=[l.strip() for l in closest(src).splitlines() if 'call void @dx.op.textureStore' in l]
 owned_stores=[l.strip() for l in c.splitlines() if 'call void @dx.op.textureStore' in l]
 assert native_stores==owned_stores and len(owned_stores)==3
 results.append(dict(variant=variant,native_sha256=hashlib.sha256(data).hexdigest(),
  owned_distance='RayTCurrent',native_distance='RayTCurrent - 0.01',position='view-space world ray endpoint',
  world_to_view_rows=[2,3,4,5],material_normal_uv_stores_preserved=True,stores=len(owned_stores)))
out={'status':'PASS','variants':results,'limits':['Static SSA audit only; not a live GPU binding audit.','Actual texture precision, producer/evaluator constants and ray pairing remain unverified by this test.','Direction mismatch flags must not be dismissed from these results alone.']}
Path(a.output).write_text(json.dumps(out,indent=2)+'\n');print(json.dumps(out))
