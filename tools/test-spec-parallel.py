import base64,pathlib,struct,sys
from spec_parallel import parallelize,fixture,instructions,SOURCE_SHA
from dxbc_transform import transform_shex
root=pathlib.Path(__file__).resolve().parents[1]
native=base64.b64decode((root/'research/spec-performance/spec-resolve.dxbc.b64').read_text())
parallel=parallelize(native)
try:parallelize(native[:-1]+bytes([native[-1]^1]));raise AssertionError('Unrecognized input accepted')
except ValueError:pass
get=lambda b: extract(b)
def extract(b):
    out=[];transform_shex(b,lambda w:out.extend(instructions(w)));return out
original=extract(native);new=extract(parallel)
# Per-hit shading/clamping remains byte-identical and occurs exactly once.
body=sum(original[143:961],[]);flat=sum(new,[])
assert sum(flat[i:i+len(body)]==body for i in range(len(flat)))==1
assert sum((i[0]&2047)==190 for i in new)==1
# Barrier must be outside every divergent IF and LOOP.
depth=0
for ins in new:
    op=ins[0]&2047
    if op in (31,48):depth+=1
    if op in (21,22):depth-=1
    if op==190:assert depth==0 and ins[0]&0x1800==0x1800
    assert depth>=0
assert depth==0
out=pathlib.Path(sys.argv[1] if len(sys.argv)>1 else 'build');out.mkdir(exist_ok=True,parents=True)
(out/'spec-native-fixture.dxbc').write_bytes(fixture(native,native))
(out/'spec-parallel-fixture.dxbc').write_bytes(fixture(parallel,native))
print('PASS hash rejection, unchanged hit shader, uniform shared-memory barrier; generated GPU differential fixtures')
