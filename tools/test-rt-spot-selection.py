"""Check selector patch structure and selected/nonselected energy bookkeeping."""
import base64,hashlib,json,math,random,struct,sys
from pathlib import Path
from rt_spot_selection import unshadowed_selection,NATIVE_SHA
root=Path(__file__).resolve().parents[1]
data=base64.b64decode((root/'assets/rt-spot/rt_spot_selection.dxbc.b64').read_text())
m=json.loads((root/'assets/rt-spot/manifest.json').read_text())['outputs']['rt_spot_selection.dxbc']
assert len(data)==m['bytes'] and hashlib.sha256(data).hexdigest()==m['sha256']
def words(data):
    for offset in struct.unpack_from('<'+'I'*struct.unpack_from('<I',data,28)[0],data,32):
        if data[offset:offset+4]==b'SHEX':
            n=struct.unpack_from('<I',data,offset+4)[0]//4
            return list(struct.unpack_from('<'+'I'*n,data,offset+8))
    raise AssertionError('No SHEX')
w=words(data)
assert w[226]==44
# Decode all instruction lengths: catches patch alignment/control-flow damage.
i=2;instructions=[]
while i<len(w):
    token=w[i];op=token&2047
    size=w[i+1] if op==53 else (token>>24)&127
    assert size>0 and i+size<=len(w)
    instructions.append((op,w[i:i+size]));i+=size
assert i==len(w)
if len(sys.argv)>1:
    original=Path(sys.argv[1]).read_bytes();assert hashlib.sha256(original).hexdigest()==NATIVE_SHA
    assert unshadowed_selection(original)==data
    native=words(original)
    # Everything after spotlight selection, including point branch, is untouched.
    assert w[-(len(native)-10565):]==native[10565:]
    assert w[2:225]==native[2:225]
    assert w[227:5686]==native[227:5686]
# Selection must not vary when map visibility changes. Unselected contributions
# must still include exactly their map visibility, and selected native+remaining
# must reconstruct the native sum if the selected pass uses map visibility.
random.seed(94)
for _ in range(10000):
    energies=[[random.random()*10 for _ in range(3)] for _ in range(random.randrange(1,10))]
    selected=max(range(len(energies)),key=lambda i:sum(energies[i]))
    for shadow in ([0.0]*len(energies),[1.0]*len(energies),[random.random() for _ in energies]):
        best=-1;chosen=None;remaining=[0.,0.,0.];held=[0.,0.,0.]
        for j,e in enumerate(energies):
            score=sum(e)*.333299994;weighted=[x*shadow[j] for x in e]
            if best<score:
                remaining=[a+b for a,b in zip(remaining,held)];held=weighted;best=score;chosen=j
            else:remaining=[a+b for a,b in zip(remaining,weighted)]
        assert chosen==selected
        for c in range(3):
            expected=sum(e[c]*v for e,v in zip(energies,shadow))
            assert math.isclose(remaining[c]+held[c],expected,rel_tol=1e-12,abs_tol=1e-12)
print('PASS selector DXBC structure, unchanged point branch when capture supplied, map-independent ranking and energy bookkeeping')
