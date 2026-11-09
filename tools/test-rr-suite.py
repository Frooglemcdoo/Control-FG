"""Check unboosted transforms and reject incomplete/mislabelled capture suites."""
import base64,copy,importlib.util
from spec_suite import variants,unboost,corrected_emission
from spec_hit_view import ROOT,optimize,view_block,op,f
from spec_parallel import instructions,dst,src
from spec_local_components import component as local
from spec_indirect_components import component as indirect
from spec_environment_sources import source_shader
from spec_components import component as components
from dxbc_transform import transform_shex

def extract(data):
    a=[];transform_shex(data,lambda w:a.extend(instructions(w)));return a
native=base64.b64decode((ROOT/'research/spec-performance/spec-resolve.dxbc.b64').read_text())
v=variants(native);a=extract(native)
assert v[0]==optimize(native) and v[1]==local(native,True) and v[5]==indirect(native,False)
gain=op(56,dst(14,'xyw'),src(14,'xyww'),f(8))
for i,volume in [(2,True),(3,False)]:
    before=extract(source_shader(native,volume));after=extract(v[i]);assert before.count(gain)==1
    assert after==[ins for ins in before if ins!=gain]
    # Cap operations retain original-total denominator/threshold, only numerator differs.
    assert a[956] in after and a[958] in after
em=extract(components(native,True));expected=[]
for ins in em:
    if ins in (a[444],a[445]):continue
    expected.extend(instructions([0,0]+view_block(a)) if ins==a[443] else [ins])
assert extract(v[4])==expected
assert all(gain not in extract(data) for data in v)
try:variants(b'unsupported')
except ValueError:pass
else:raise AssertionError('unknown shader accepted')
spec=importlib.util.spec_from_file_location('collector',ROOT/'tools/collect-rr-suite.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
frames=[]
for step in range(1,9):
 for pair in range(2):
  frames.append((None,dict(suite_step=step,suite_samples=16 if step==8 else 4,suite_raw_rr_clamp=step==2,pair_index=pair,frame=100*step+pair,previous_frame=100*step if pair else 0,consecutive=bool(pair),rr_active=True,rr_result=1,reset=0,output_captured=True,resources=[dict(copied=True)])))
m.validate(frames)
for key,value in [('rr_active',False),('rr_result',0),('reset',1),('output_captured',False),('suite_samples',2),('suite_raw_rr_clamp',True),('frame',999),('consecutive',False)]:
 bad=copy.deepcopy(frames);bad[1][1][key]=value
 try:m.validate(bad)
 except ValueError:pass
 else:raise AssertionError(key)
for bad in [frames[:-1],frames+frames[:1]]:
 try:m.validate(bad)
 except ValueError:pass
 else:raise AssertionError('incomplete/duplicate accepted')
print('PASS: exact unboosted transforms, R184 baseline, emission V, cap basis and suite validation failures')
