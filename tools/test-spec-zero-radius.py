"""Conservative CFG liveness proof for the two guarded pure arithmetic blocks."""
import base64,re
from pathlib import Path
from spec_parallel import instructions,inst,src
from dxbc_transform import transform_shex
from spec_serial_lifetimes import optimize as prior
from spec_zero_radius import optimize
root=Path(__file__).resolve().parents[1]
native=base64.b64decode((root/'research/spec-performance/spec-resolve.dxbc.b64').read_text())
lines=[s.strip() for s in (root/'research/spec-performance/spec-resolve.asm').read_text().split('cs_5_1\n')[1].splitlines() if s.strip() and not s.strip().startswith('//')]
def extract(data):
 a=[];transform_shex(data,lambda w:a.extend(instructions(w)));return a
original=extract(native);assert len(original)==len(lines)==984
# Conservative component reads: include every explicitly swizzled component.
def components(s):
 return {f'r{r}.{c}' for r,cs in re.findall(r'\br(\d+)\.([xyzw]+)',s) for c in cs}
reads=[];writes=[]
no_dest={'if_nz','if_z','breakc_z','breakc_nz','break','endif','else','loop','endloop','ret','store_uav_typed'}
for line in lines:
 op=line.split()[0]
 if op in no_dest or op.startswith('dcl_'):
  writes.append(set());reads.append(components(line));continue
 operands=line.split(' ',1)[1]
 parts=operands.split(',')
 n=2 if op in ('imul','umul','udiv','sincos') else 1
 writes.append(components(','.join(parts[:n])))
 reads.append(components(','.join(parts[n:])))
# Exact structured-control successors, including loop backedges and break exits.
stack=[];ends={};elses={};breaks={}
for i,line in enumerate(lines):
 op=line.split()[0]
 if op in ('if_nz','if_z','loop'):stack.append(i)
 elif op=='else':elses[stack[-1]]=i
 elif op in ('endif','endloop'):
  start=stack.pop();ends[start]=i
 elif op.startswith('break'):
  breaks[i]=next(j for j in reversed(stack) if lines[j]=='loop')
assert not stack
edges=[]
for i,line in enumerate(lines):
 op=line.split()[0];nexts={i+1} if i+1<len(lines) else set()
 if op in ('if_nz','if_z'):nexts.add(elses.get(i,ends[i])+1)
 elif op=='else':nexts={ends[next(k for k,v in elses.items() if v==i)]+1}
 elif op=='endloop':nexts={next(k for k,v in ends.items() if v==i)+1}
 elif op.startswith('break'):nexts.add(ends[breaks[i]]+1)
 elif op=='ret':nexts=set()
 edges.append(nexts)
live=[set() for _ in lines]
while True:
 changed=False
 for i in reversed(range(len(lines))):
  value=reads[i]|(set().union(*(live[j] for j in edges[i]))-writes[i])
  if value!=live[i]:live[i]=value;changed=True
 if not changed:break
for start,end,condition,result,selector in [
 (568,608,'r28.w','r21.z','movc r21.z, r28.w, r21.z, r22.y'),
 (857,897,'r26.w','r20.w','movc r20.w, r26.w, r20.w, r22.y')]:
 assert lines[end]==selector
 assert lines[start-1].startswith('ne '+condition+',')
 changed=set().union(*writes[start:end])
 assert condition not in changed
 assert changed & live[end] == {result},(start,changed & live[end])
 assert all(l.split()[0] in {'mul','div','add','mul_sat','mad','sqrt','lt','and','min','movc'} for l in lines[start:end])
 # Source blocks are straight-line pure arithmetic, and their sole live result
 # is discarded by the existing bitwise movc whenever the new guard is false.
 assert len(lines[start:end])==40
 print('PASS guard',condition,'sole live result',result,'discarded by native selector when false')
new=extract(optimize(native));expected=extract(prior(native))
# Independent stripping of the four added control instructions recovers R172.
guards=[inst('if_',src(28,'w'),flags=0x40000),inst('if_',src(26,'w'),flags=0x40000)]
# Original load movement shifts both guarded blocks by two instructions.
assert new[566]==guards[0] and new[607]==inst('endif')
assert new[857]==guards[1] and new[898]==inst('endif')
assert [x for i,x in enumerate(new) if i not in (566,607,857,898)]==expected
try:optimize(native[:-1]);raise AssertionError('unknown input accepted')
except ValueError:pass
print('PASS exact R172 instruction preservation plus four guards; sample loop/clamp/reduction unchanged')
from spec_zero_radius import fixture
out=root/'build';out.mkdir(exist_ok=True)
(out/'spec-area-native-fixture.dxbc').write_bytes(fixture(native,False))
(out/'spec-area-guarded-fixture.dxbc').write_bytes(fixture(native,True))
for guarded in (False,True):extract(fixture(native,guarded))
print('Generated native/guarded WARP fixtures retaining actual area-light arithmetic')
