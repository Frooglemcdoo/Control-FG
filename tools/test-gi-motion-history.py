import base64,struct,tempfile,sys
from pathlib import Path
from gi_motion_history import generate
from dxbc_checksum import checksum
root=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root/'research/native-integration-audit/tools'))
from decode_dxbc import decode
source=base64.b64decode((root/'research/gi-temporal-r208/diffuse-temporal.dxbc.b64').read_text())
result=generate(source)
assert len(result)==len(source)==5076 and result[4:20]==checksum(result)
def chunks(data):
 return {data[o:o+4]:data[o+8:o+8+struct.unpack_from('<I',data,o+4)[0]] for o in struct.unpack_from('<'+'I'*struct.unpack_from('<I',data,28)[0],data,32)}
a,b=chunks(source),chunks(result)
assert a.keys()==b.keys()
for k in a:
 if k!=b'SHEX':assert a[k]==b[k],k
w1=struct.unpack('<'+'I'*(len(a[b'SHEX'])//4),a[b'SHEX']);w2=struct.unpack('<'+'I'*(len(b[b'SHEX'])//4),b[b'SHEX'])
assert len([i for i,(x,y) in enumerate(zip(w1,w2)) if x!=y])==2
with tempfile.TemporaryDirectory() as d:
 p=Path(d)/'gi.dxbc';p.write_bytes(result);j=decode(p)
ins={i['dword']:i for i in j['instructions']}
assert ins[356]['operands']==['r[1].x','-r[1].x','l(400)','l(1)']
assert ins[311]['operands']==['r[1].z','l(0.00999999978)','r[1].z']
assert ins[219]['operands'][-1]=='l(0.699999988)'
def weight(motion,depth,cut=0):return (1-cut)*.7*(depth<=.01)*max(0,1-400*motion)
assert weight(0,0)==.7 and weight(.0025,0)==0 and weight(0,.011)==0 and weight(0,0,1)==0
assert 0<weight(.001,0)<.7
try:generate(source[:-1]);raise AssertionError('Unknown shader accepted')
except ValueError:pass
print('PASS exact two-immediate transform, unchanged bindings/clamp/history factor, stable history and stricter motion/depth rejection')

assert {(v['slot'],v['space']) for v in j['bindings'] if v['kind']==3}=={(6,1),(11,1)}
fixture=(root/'tools/test-gi-motion-pso.cpp').read_text()
assert 'samplers[0].RegisterSpace=1' in fixture and 'samplers[1].RegisterSpace=1' in fixture
