"""Independent binary, component-hazard and structured-control audit for R177."""
import base64,re
from collections import Counter
from pathlib import Path
from dxbc_transform import transform_shex
from spec_parallel import instructions
from spec_serial_lifetimes import optimize as baseline
from spec_probe_lifetimes import optimize
root=Path(__file__).resolve().parents[1]
data=base64.b64decode((root/'research/spec-performance/spec-resolve.dxbc.b64').read_text())
lines=[s.strip() for s in (root/'research/spec-performance/spec-resolve.asm').read_text().split('cs_5_1\n')[1].splitlines() if s.strip() and not s.strip().startswith('//')]
def extract(blob):
    result=[];transform_shex(blob,lambda w:result.extend(instructions(w)));return result
native=extract(data)
assert len(native)==len(lines)==984
assert lines[201]=='ld_structured r29.xyz, r10.w, l(0), T16[16].xyzx'
assert lines[202]=='ld_structured r30.xyz, r10.w, l(12), T16[16].xyzx'
assert lines[289]=='mov r32.x, r29.x'
for i,reg,offset in ((201,29,0),(202,30,12)):
    assert native[i][1:3]==[0x100072,reg]
    assert native[i][3:5]==[0x10003a,10]
    assert native[i][5:7]==[0x4001,offset]
    assert native[i][7:]==[0x207246,16,16]
for line in lines[203:289]:
    for reg,comps in re.findall(r'\br(\d+)\.([xyzw]+)',line):
        assert not (int(reg) in (29,30) and set(comps)&set('xyz')),line
    op=line.split()[0]
    # Include both destinations for multiply/divide instructions.
    n=2 if op in ('imul','umul','udiv','sincos') else 1
    dest=','.join(line.split(' ',1)[-1].split(',')[:n])
    assert not re.search(r'\br10\.[xyzw]*w',dest),line
    assert not op.startswith(('store','atomic','sync','ret','discard','call','continue')),line
stack=[];sites={}
for i,line in enumerate(lines):
    sites[i]=tuple(stack);op=line.split()[0]
    if op in ('if_nz','if_z','loop'):stack.append((op,i))
    elif op=='else':stack[-1]=('else',i)
    elif op in ('endif','endloop'):stack.pop()
    elif 203<=i<289 and op.startswith('break'):
        # A crossed break must exit only a nested loop, never bypass the sink.
        assert next(j for kind,j in reversed(stack) if kind=='loop')>202
assert sites[201]==sites[202]==sites[289] and not stack
old=extract(baseline(data));new=extract(optimize(data))
assert Counter(map(tuple,new))==Counter(map(tuple,native))
# Undo only the new move and recover the accepted R172/R176 shader exactly.
assert new[:201]+new[287:289]+new[201:287]+new[289:]==old
for bad in (data[:-1],data[:-1]+bytes([data[-1]^1])):
    try:optimize(bad)
    except ValueError:pass
    else:raise AssertionError('Unknown source accepted')
print('PASS: exact binary operands, unchanged address/destinations, same control scope, nested-break safety, reversible permutation, unknown-source rejection')
