"""Check the actual inserted PHIs across exhaustive 8-ray rejection patterns.

The transformer proves the native fallback has no sample-dependent inputs or
side effects. This test treats that pure function's result as three opaque
values and executes the patched PHIs, preserving distinct accepted directions.
It is a control/data-flow proof, not a GPU performance or DXR execution test.
"""
import itertools
import random
import re
from pathlib import Path
from spec_retry_reuse import ROOT, ENTRY, transform

source = (ROOT/'research/native-integration-audit/reflection-000.ll').read_text()
patched = transform(source)
phis = dict(re.findall(r'^  (%[\w.]+) = phi (?:float|i1) (.*)$', patched, re.M))

def phi(name, predecessor, env):
    choices = re.findall(r'\[ ([^,]+), (%"[^"]+"|%[\w.]+) \]', phis[name])
    values = [value for value, block in choices if block == predecessor]
    assert len(values) == 1, (name, predecessor)
    value = values[0]
    if value.startswith('%'): return env[value]
    return {'true': True, 'false': False, '0.000000e+00': 0.0}[value]

cases = list(itertools.product((False, True), repeat=8))
for n in (1, 16, 64):
    cases += [tuple([False]*n), tuple([True]*n)]
    for i in range(n):
        cases += [tuple(j == i for j in range(n)), tuple(j >= i for j in range(n))]
rng = random.Random(176)
cases += [tuple(rng.choice((False,True)) for _ in range(16)) for _ in range(1000)]
for rejected in cases:
    env = {}; pred = '%.lr.ph30.preheader'; calls = 0
    for sample, fail in enumerate(rejected):
        outer = {name:phi(name,pred,env) for name in ['%RetryCached']+[f'%RetryCache{i}' for i in range(3)]}
        env.update(outer)
        # Accepted and fallback values are deliberately distinct per component.
        fallback = (701,702,703)
        for i, name in enumerate(('%.i0291','%.i1292','%.i2293')):
            env[name] = sample*10+i
        if not fail: edge='%'+ENTRY
        elif env['%RetryCached']: edge='%RetryReuse'
        else:
            edge='%._crit_edge.loopexit'; calls+=1
            for name,value in zip(('%.i0324.lcssa','%.i1325.lcssa','%.i2326.lcssa'),fallback):env[name]=value
        actual = tuple(phi(f'%vReflectionDirectionInView.0.lcssa.i{i}',edge,env) for i in range(3))
        assert actual == (fallback if fail else tuple(sample*10+i for i in range(3)))
        nexts = {name:phi(name,edge,env) for name in ['%RetryNextCached']+[f'%RetryNext{i}' for i in range(3)]}
        env.update(nexts);pred='%._crit_edge'
    assert calls == int(any(rejected))

# Fail closed when a native change makes retry input sample-dependent, introduces
# memory side effects, changes the cap or invalidates the assumed variant layout.
for before, after in [('%293 = uitofp i32 %270','%293 = uitofp i32 %272'),
                      ('icmp ult i32 %294, 100','icmp ult i32 %294, 99'),
                      ('  %293 =', '  store i32 0, i32* %350\n  %293 =')]:
    assert before in source
    try: transform(source.replace(before,after,1))
    except AssertionError: pass
    else: raise AssertionError('Unsafe source mutation accepted')
# No part of the library outside raygen changes, including all hit handlers.
a=source.index('define void @"\\01?reflectionRayGeneration@@');b=source.index('\n}',a)+2
c=patched.index('define void @"\\01?reflectionRayGeneration@@');d=patched.index('\n}',c)+2
assert source[:a]==patched[:c] and source[b:]==patched[d:]
print(f'PASS: {len(cases)} PHI/control-flow cases; dependency/side-effect proof; mutation rejection; unchanged hit library and ray arithmetic')
