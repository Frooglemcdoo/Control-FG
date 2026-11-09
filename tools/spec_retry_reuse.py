"""Memoize the sample-independent rejection fallback in native reflection variant 0.

No ray is removed. Cache lives only for one raygen invocation. First rejected
sample executes the exact original fallback, including its 100-iteration cap.
"""
import argparse
import hashlib
import json
import re
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE_SHA = 'c2e4c0c4b48b191ba281595dcf11c7962c5c05c615cbe036ca6e8616c09fd30e'
ENTRY = '"\\01?internal.getImportanceRay@@YA?AV?$vector@M$02@@V1@0MV?$vector@M$01@@@Z.exit"'


def transform(source):
    start = source.index('define void @"\\01?reflectionRayGeneration@@')
    end = source.index('\n}', start) + 2
    body = source[start:end]
    original = body
    # Transitive SSA proof: fallback arithmetic can only depend on values defined
    # outside the sample loop, the immutable frame index, or its own retry index.
    loop = body[body.index('\n.lr.ph30:'):body.index('\n._crit_edge.31.loopexit:')]
    retry = loop[loop.index('\n.lr.ph.preheader:'):loop.index('\n._crit_edge:')]
    defs = dict(re.findall(r'^  (%[\w.]+) = (.*)$', loop, re.M))
    retry_defs = dict(re.findall(r'^  (%[\w.]+) = (.*)$', retry, re.M))
    assert defs['%270'] == 'extractvalue %dx.types.CBufRet.i32 %269, 1'
    assert '@dx.op.cbufferLoadLegacy.i32(i32 59, %dx.types.Handle %sys_constants, i32 69)' in defs['%269']
    assert not re.search(r'@dx.op.(?:texture|buffer|rawBuffer|atomic|traceRay)|\b(?:load|store)\b', retry)
    assert 'icmp ult i32 %294, 100' in retry
    seen = set()
    def visit(value):
        if value in seen or value not in defs:
            return
        seen.add(value)
        assert value in retry_defs or value in ('%269', '%270'), f'Sample-varying fallback input: {value}'
        rhs = defs[value].split(';')[0]
        # PHI predecessor block labels are not SSA data dependencies.
        if rhs.startswith('phi '):
            dependencies = re.findall(r'\[ (%[\w.]+),', rhs)
        else:
            dependencies = re.findall(r'%[\w.]+', rhs)
        for dependency in dependencies:
            visit(dependency)
    for value in retry_defs:
        visit(value)

    old = '  %reflectionRayIndex.029 = phi i32 [ %338, %._crit_edge ], [ 0, %.lr.ph30.preheader ]\n'
    assert body.count(old) == 1
    phis = '  %RetryCached = phi i1 [ %RetryNextCached, %._crit_edge ], [ false, %.lr.ph30.preheader ]\n'
    for i in range(3):
        phis += f'  %RetryCache{i} = phi float [ %RetryNext{i}, %._crit_edge ], [ 0.000000e+00, %.lr.ph30.preheader ]\n'
    body = body.replace(old, old + phis)
    branch = '  br i1 %292, label %.lr.ph.preheader, label %._crit_edge'
    assert body.count(branch) == 1
    body = body.replace(branch, '  br i1 %292, label %RetryGate, label %._crit_edge')
    gate = '\nRetryGate:\n  br i1 %RetryCached, label %RetryReuse, label %.lr.ph.preheader\n\nRetryReuse:\n  br label %._crit_edge\n'
    body = body.replace('\n.lr.ph.preheader:', gate + '\n.lr.ph.preheader:')
    for i in range(3):
        pattern = rf'(^  %vReflectionDirectionInView.0.lcssa.i{i} = phi float .*?)(\n)'
        body, count = re.subn(pattern, rf'\1, [ %RetryCache{i}, %RetryReuse ]\2', body, flags=re.M)
        assert count == 1
    marker = '  %319 = call '
    nexts = f'  %RetryNextCached = phi i1 [ true, %._crit_edge.loopexit ], [ true, %RetryReuse ], [ %RetryCached, %{ENTRY} ]\n'
    for i, value in enumerate(('%.i0324.lcssa', '%.i1325.lcssa', '%.i2326.lcssa')):
        nexts += f'  %RetryNext{i} = phi float [ {value}, %._crit_edge.loopexit ], [ %RetryCache{i}, %RetryReuse ], [ %RetryCache{i}, %{ENTRY} ]\n'
    assert body.count(marker) == 1
    body = body.replace(marker, nexts + marker)
    # Preds comments from dumpbin are cosmetic and stale after adding CFG edges.
    result = source[:start] + body + source[end:]
    # Existing computations, trace operands, stores, handlers and metadata remain.
    def arithmetic(text):
        return [line for line in text.splitlines() if line.startswith('  ') and
                not any(x in line for x in (' = phi ', '  br ', '%Retry'))]
    assert arithmetic(original) == arithmetic(body)
    assert retry in body  # complete fallback body unchanged, including final PHIs
    return result


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--dxc', required=True)
    p.add_argument('--assembler', required=True)
    a = p.parse_args()
    build = ROOT/'build'; build.mkdir(exist_ok=True)
    native = ROOT/'research/hit-distance/evidence/original-shaders/rt_reflection-000.dxbc'
    assert hashlib.sha256(native.read_bytes()).hexdigest() == SOURCE_SHA
    source = subprocess.check_output([a.dxc, '-dumpbin', str(native)], text=True)
    (build/'spec-retry-original.ll').write_text(source)
    ir = build/'spec-retry-reuse.ll'; ir.write_text(transform(source))
    output = build/'spec-retry-reuse.dxil'
    subprocess.run([a.assembler, str(ir), str(output), str(Path(a.dxc).parent/'dxcompiler.dll')], check=True)
    data = output.read_bytes()
    assert data[:4] == b'DXBC' and any(data[4:20])
    (build/'spec_retry_reuse.generated.h').write_text('#pragma once\nstatic const unsigned char kSpecRetryReuseShader[]={\n' + ','.join(f'0x{x:02x}' for x in data) + '\n};\n')
    report = dict(source_sha256=SOURCE_SHA, sha256=hashlib.sha256(data).hexdigest(), bytes=len(data),
                  variant=0, cached_values=4, ray_count_changed=False, retry_cap=100, dxil_validated=True,
                  hardware_performance_measured=False)
    (build/'spec-retry-validation.json').write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps(report))

if __name__ == '__main__':
    main()
