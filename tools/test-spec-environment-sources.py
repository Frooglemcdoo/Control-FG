"""Audit route provenance, dominance and reversible source isolation."""
import base64
from spec_environment_sources import ROOT,extract,source_shader,tag,gate
from spec_indirect_components import component

data=base64.b64decode((ROOT/'research/spec-performance/spec-resolve.dxbc.b64').read_text())
a=extract(data);baseline=extract(component(data,True))
lines=[x.strip() for x in (ROOT/'research/spec-performance/spec-resolve.asm').read_text().split('cs_5_1\n')[1].splitlines() if x.strip() and not x.strip().startswith('//')]
assert lines[366:369]==['and r7.w, r7.w, r9.w','if_nz r7.w','lt r7.w, l(0.000000), r8.w']
assert lines[377]=='else' and lines[403]=='endif'
assert lines[370]=='div r29.xyz, r26.xyzx, r8.wwww'
assert lines[401]=='sample_l r23.xyz, r23.xyzx, T8[8].xyzw, S2[10], r7.w'
assert lines[402]=='mul r29.xyz, r23.xyzx, CB1[1][15].yyyy'
assert lines[955]=='endif' and lines[956].startswith('dp3 r6.w, r29.xyzx,')
# Both routes assign the tag on every loop iteration before hit/miss shading.
# No branch exits the source region before its assignment.
assert not any(x.startswith(('break','continue','ret')) for x in lines[367:404])
for volume in (False,True):
    b=extract(source_shader(data,volume))
    for anchor,addition in [(a[368],tag(volume)),(a[378],tag(not volume)),(a[956],gate())]:
        assert b.count(anchor)==1 and b.count(addition)==1
        pos=b.index(anchor);assert b[pos-1]==addition;b.pop(pos-1)
    assert b[41]==[0x02000068,45];b[41]=baseline[41]
    assert b==baseline # cap basis, BRDF, rays, all texture reads unchanged
    for bad in (data[:-1],data[:-1]+bytes([data[-1]^1])):
        try:source_shader(bad,volume)
        except ValueError:pass
        else:raise AssertionError('Unknown source accepted')
# Decode the selection operands, independent of transform composition.
g=gate();assert g[0]&2047==55 and g[2]==43 and g[4]==44 and g[6]==43 and g[-1]==0
for routed_volume in (False,True):
    selected=[]
    for volume in (False,True):
        token=tag(volume if routed_volume else not volume)
        assert token[0]&2047==54 and token[2]==44
        selected.append(token[-1]!=0)
    assert sum(selected)==1 and selected[int(routed_volume)]
print('PASS native volume/cube provenance, per-sample tag dominance, disjoint exhaustive selection, exact R187 recovery and unknown-source rejection')
