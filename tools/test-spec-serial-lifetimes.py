"""Independent movement/hazard audit against the pinned native disassembly."""
import base64
from collections import Counter
from pathlib import Path
import re
from dxbc_transform import transform_shex
from spec_parallel import instructions
from spec_serial_lifetimes import optimize

root = Path(__file__).resolve().parents[1]
data = base64.b64decode((root/'research/spec-performance/spec-resolve.dxbc.b64').read_text())
asm = (root/'research/spec-performance/spec-resolve.asm').read_text().split('cs_5_1\n')[1]
lines = [s.strip() for s in asm.splitlines() if s.strip() and not s.strip().startswith('//')]

def extract(blob):
    out = []
    transform_shex(blob, lambda w: out.extend(instructions(w)))
    return out

original = extract(data)
assert len(lines) == len(original) == 984
assert lines[405] == 'ld_structured r23.xyzw, r5.w, l(48), T4[4].xyzw'
assert lines[406] == 'ld_structured r24.xyz, r5.w, l(100), T4[4].xyzx'
assert lines[943].startswith('sample_l r16.xyz,')
# Check binary operands independently of disassembly: destination, material ID,
# byte offset and read-only resource. SM5.1 structured SRV has two indices.
for i, reg, mask, offset, swizzle in [(405,23,15,48,0xe46),(406,24,7,100,0x246)]:
    ins = original[i]
    assert ins[1:3] == [0x100002 | (mask << 4), reg]
    assert ins[3:5] == [0x10003a, 5]  # scalar r5.w
    assert ins[5:7] == [0x4001, offset]
    assert ins[7:] == [0x207000 | swizzle, 4, 4]
# Conservative read/write hazard check (including all swizzled components).
for line in lines[407:943]:
    assert not re.search(r'\br23\.', line), line
    for comps in re.findall(r'\br24\.([xyzw]+)', line):
        assert not set(comps) & set('xyz'), line
    dest = line.split(' ',1)[1].split(',')[0] if ' ' in line else ''
    assert not re.search(r'\br5\.[xyzw]*w', dest), line
    assert not line.startswith(('store_', 'atomic_', 'sync', 'ret', 'discard', 'call')), line
stack=[]
sites={}
for i,line in enumerate(lines):
    sites[i]=tuple(stack)
    op=line.split()[0]
    if op in ('if_nz','if_z','loop'):stack.append((op,i))
    elif op == 'else':stack[-1]=('else',i)
    elif op in ('endif','endloop'):stack.pop()
assert sites[405] == sites[406] == sites[943]
assert not stack
new = extract(optimize(data))
assert Counter(map(tuple,new)) == Counter(map(tuple,original))
# Undo the move: all instruction bytes and order must be exactly restored.
assert new[:405] + new[941:943] + new[405:941] + new[943:] == original
for mutated in (data[:-1], data[:-1]+bytes([data[-1]^1])):
    try: optimize(mutated)
    except ValueError: pass
    else: raise AssertionError('Unknown shader accepted')
print('PASS binary operands, destination/address hazards, control-flow scope, exact reversible instruction permutation, unknown-input rejection')
