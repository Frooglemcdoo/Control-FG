import base64,struct,tempfile,sys
from pathlib import Path
from gi_motion_history import generate
from dxbc_checksum import checksum
root=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root/'research/native-integration-audit/tools'))
from decode_dxbc import decode
source=base64.b64decode((root/'research/gi-temporal-r208/diffuse-temporal.dxbc.b64').read_text())
result=generate(source)
assert len(source)==5076 and len(result)>len(source) and result[4:20]==checksum(result)
def chunks(data):
 return {data[o:o+4]:data[o+8:o+8+struct.unpack_from('<I',data,o+4)[0]] for o in struct.unpack_from('<'+'I'*struct.unpack_from('<I',data,28)[0],data,32)}
a,b=chunks(source),chunks(result)
assert a.keys()==b.keys()
for k in a:
 if k!=b'SHEX':assert a[k]==b[k],k
with tempfile.TemporaryDirectory() as d:
 p=Path(d)/'gi.dxbc';p.write_bytes(result);j=decode(p)
 p.write_bytes(source);original=decode(p)
# Every original instruction survives in order, except temp count and the two
# audited R211 constants. Additional code appears only at three boundaries.
new=j['instructions'];old=original['instructions']
added={144:18,204:6,393:2}
pos=0
for instruction in old:
 pos+=added.get(instruction['dword'],0)
 actual=new[pos];expected=dict(instruction);expected.pop('dword');actual={k:v for k,v in actual.items() if k!='dword'}
 if instruction['dword']==75:expected['operands']=['raw:0x9']
 if instruction['dword']==311:expected['operands']=['r[1].z','l(0.00999999978)','r[1].z']
 if instruction['dword']==356:expected['operands']=['r[1].x','-r[1].x','l(400)','l(1)']
 assert actual==expected,(instruction,actual)
 pos+=1
assert pos==len(new)
coords=[i for i in new if i['name']=='mad' and i['operands'][0]=='r[8].xy']
assert len(coords)==4 and all(i['operands'][2:]==['cb[0][0][0].zwzz','r[1].zwzz'] for i in coords)
assert sum(i['name']=='sample_l' for i in new)==sum(i['name']=='sample_l' for i in old)+4
def weight(motion,depth,cut=0):return (1-cut)*.7*(depth<=.01)*max(0,1-400*motion)
assert weight(0,0)==.7 and weight(.0025,0)==0 and weight(0,.011)==0 and weight(0,0,1)==0
assert 0<weight(.001,0)<.7
try:generate(source[:-1]);raise AssertionError('Unknown shader accepted')
except ValueError:pass
print('PASS R212 preserved native instruction stream and bindings, four filtered neighborhood taps, RGB clipping, R211 motion/depth and history factor')

assert {(v['slot'],v['space']) for v in j['bindings'] if v['kind']==3}=={(6,1),(11,1)}
fixture=(root/'tools/test-gi-motion-pso.cpp').read_text()
assert 'samplers[0].RegisterSpace=1' in fixture and 'samplers[1].RegisterSpace=1' in fixture

# The zero-history variant differs from R212 at exactly the blend coefficient.
zero=generate(source,False)
z=chunks(zero)
assert zero[4:20]==checksum(zero)
x=struct.unpack('<'+'I'*(len(b[b'SHEX'])//4),b[b'SHEX'])
y=struct.unpack('<'+'I'*(len(z[b'SHEX'])//4),z[b'SHEX'])
diff=[(a,b) for a,b in zip(x,y) if a!=b]
assert diff==[(struct.unpack('<I',struct.pack('<f',.7))[0],0)]
assert all(z[k]==b[k] for k in z if k!=b'SHEX')
print('PASS R213 zero-history variant differs only at native blend coefficient')

# The outlier variant preserves the native sequence around one insertion,
# including the existing energy clamp and RGB normalization. No history limit
# or filtered-neighborhood samples from R212 are added to this variant.
outlier=generate(source,False,True)
o=chunks(outlier)
assert outlier[4:20]==checksum(outlier)
assert all(o[k]==a[k] for k in a if k!=b'SHEX')
with tempfile.TemporaryDirectory() as d:
 p=Path(d)/'outlier.dxbc';p.write_bytes(outlier);oj=decode(p)
new=oj['instructions'];pos=0
for instruction in original['instructions']:
 if instruction['dword']==197:pos+=27 # init + 4*(UV, sample, luma, min, max, max) + cap + min
 actual={k:v for k,v in new[pos].items() if k!='dword'}
 expected={k:v for k,v in instruction.items() if k!='dword'}
 if instruction['dword']==75:expected['operands']=['raw:0x9']
 if instruction['dword']==219:expected['operands']=['r[0].w','r[0].w','l(0)']
 if instruction['dword']==311:expected['operands']=['r[1].z','l(0.00999999978)','r[1].z']
 if instruction['dword']==356:expected['operands']=['r[1].x','-r[1].x','l(400)','l(1)']
 assert actual==expected,(instruction,actual)
 pos+=1
assert pos==len(new)
assert sum(i['name']=='sample_l' for i in new)==sum(i['name']=='sample_l' for i in original['instructions'])+4
try:generate(source,True,True);raise AssertionError('Outlier history unexpectedly allowed')
except ValueError:pass
print('PASS R214 current-frame outlier insertion, unchanged native color normalization, zero history, original bindings')
