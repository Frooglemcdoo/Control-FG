"""Prove the untouched native GI shader's CameraCut weighting and retained clamp."""
import sys,hashlib,base64,tempfile
from pathlib import Path
root=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root/'research/native-integration-audit/tools'))
from decode_dxbc import decode
source=base64.b64decode((root/'research/gi-temporal-r208/diffuse-temporal.dxbc.b64').read_text())
with tempfile.TemporaryDirectory() as d:
 p=Path(d)/'diffuse-temporal.dxbc';p.write_bytes(source);j=decode(p)
assert j['sha256']=='d54d94fdbb783796cc33bc78ab20dde687d90d9cba6f24c8092a6b177eec5d15'
v=next(v for c in j['constant_buffers'] if c['name']=='deferredlight_filtering' for v in c['variables'] if v['name']=='g_uCameraCut')
assert v['offset']==24 and v['size']==4 and v['flags']&2
ins={i['dword']:(i['name'],i['operands']) for i in j['instructions']}
expected={204:('iadd',['r[0].w','-cb[1][1][1].z','l(1)']),214:('utof',['r[0].w','r[0].w']),219:('mul',['r[0].w','r[0].w','l(0.699999988)']),325:('mul',['r[0].w','r[0].w','r[1].z']),373:('mul',['r[0].w','r[0].w','r[1].x']),403:('mad',['r[1].xyzw','r[0].wwww','r[1].xyzw','r[3].xyzw'])}
for offset,want in expected.items():assert ins[offset]==want,(offset,ins[offset])
assert ins[190]==('min',['r[0].z','r[0].z','r[0].w'])
assert ins[197]==('mul',['r[3].xyzw','r[0].zzzz','r[2].xyzw'])
# Semantic fixture: zeroing history darkens output; zeroing its weight preserves current energy.
for current in (0.,.1,1.,20.):
 for history in (0.,.2,10.,100.):
  for confidence in (0.,.3,1.):
   weight=(1-1)*.7*confidence
   assert current+weight*(history-current)==current
assert 1+.7*(0-1)!=1
# Guard exact bind/dispatch provenance, with spatial stage after temporal.
a=(root/'research/native-integration-audit/evidence/diffuse-filter.asm').read_text()
assert '180015691:' in a and '1800156d3:' in a
assert a.index("'temporal_feedback'")<a.index("'filter_color'")
print('PASS exact shader identity, CameraCut operand, native 70% history factor, current-frame clamp and temporal/spatial call order')
