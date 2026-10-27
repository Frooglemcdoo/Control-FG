"""Independent token/control-flow and cap-basis audit for R182 isolation."""
import base64
import random
from pathlib import Path
from spec_components import component
from spec_parallel import instructions, src, dst, inst, imm
from dxbc_transform import transform_shex

root=Path(__file__).resolve().parents[1]
data=base64.b64decode((root/'research/spec-performance/spec-resolve.dxbc.b64').read_text())
lines=[x.strip() for x in (root/'research/spec-performance/spec-resolve.asm').read_text().split('cs_5_1\n')[1].splitlines()
       if x.strip() and not x.strip().startswith('//')]
def extract(blob):
    out=[];transform_shex(blob,lambda w:out.extend(instructions(w)));return out
a=extract(data)
assert len(a)==len(lines)==984
for i,text in {404:'if_z r6.w',953:'mad r14.xyw, r14.xyxw, r16.xyxz, r15.xyxz',
               954:'add r29.xyz, r19.xyzx, r14.xywx',955:'endif',
               956:'dp3 r6.w, r29.xyzx, l(0.333300, 0.333300, 0.333300, 0.000000)',
               957:'div r14.xyw, r29.xyxz, r6.wwww',
               960:'movc r14.xyw, r3.wwww, r14.xyxw, r29.xyxz'}.items():
    assert lines[i]==text
for emission in (False,True):
    b=extract(component(data,emission))
    assert len(b)==986
    # Undo additions and operand substitutions: recover the exact accepted
    # baseline permutation. No native shading, loop or resource token changes.
    expected=a[:405]+a[407:943]+a[405:407]+a[943:]
    additions=[inst('mov',dst(43,'xyz'),imm(0) if emission else src(29,'xyzx')),
               inst('mov',dst(43,'xyz'),src(19,'xyzx') if emission else src(14,'xywx'))]
    for ins in additions:
        assert b.count(ins)==1;b.remove(ins)
    assert b[41]==[0x02000068,44];b[41]=a[41]
    assert b[957]==a[957][:4]+[43]+a[957][5:]
    assert b[960]==a[960][:-1]+[43]
    b[957]=a[957];b[960]=a[960]
    assert b==expected
    for bad in (data[:-1], data[:-1]+bytes([data[-1]^1])):
        try:component(bad,emission)
        except ValueError:pass
        else:raise AssertionError('Unknown source accepted')
# Separate mathematical check of the attribution rule. Both components use
# TOTAL luminance for scaling, not their own luminance. Zero-total native NaN
# behavior is intentionally unchanged and is not claimed fixed here.
rng=random.Random(182)
for _ in range(10000):
    e=[rng.random()*100 for _ in range(3)];s=[rng.random()*100 for _ in range(3)]
    total=[x+y for x,y in zip(e,s)];lum=sum(total)*.3333;cap=rng.uniform(0,200)
    for enabled in (False,True):
        def f(v):return [x/lum*min(lum,cap) if enabled else x for x in v]
        assert all(abs(x+y-z)<1e-10 for x,y,z in zip(f(e),f(s),f(total)))
print('PASS exact token recovery, component swizzles, unchanged total cap basis, misses, unknown-source rejection and additive pre-filter attribution')
