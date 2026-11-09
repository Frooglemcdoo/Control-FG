"""Reuse the first of four provably identical centered spotlight traces.

Only the hash-locked authored-clip shader is accepted. No sampling, clipping,
visibility aggregation, hit handler, point-ray or filter instructions change.
"""
import argparse
import base64
import hashlib
import json
import re
import subprocess
from pathlib import Path

BASE_SHA = 'b025748befa88c52e683cf120e59a4748a1c9ce958559421d70f3c8c2fde52ec'
TRACE = '  call void @dx.op.traceRay.struct.ShadowHitData('


def reuse_centered_rays(source):
    lines = source.splitlines(keepends=True)
    assert '%struct.ShadowHitData = type { float }' in source
    # Every disk radius is zero; nonzero-radius variants must never use this transform.
    for i in range(4):
        assert source.count(f'%SpotDisk{i}ScaledRadius = fmul fast float %SpotDisk{i}Radius, 0.000000e+00') == 1
    # All four segment expressions (including authored clipping and fallbacks)
    # are identical after renaming their index. Their disk offsets are zero.
    segments = []
    for i in range(4):
        segment = [line.split(';')[0].strip() for line in lines if re.match(rf'  %SpotSegment{i}\w+ =', line)]
        segments.append(re.sub(r'Spot(Segment|Disk)[0-3]', r'Spot\1N', '\n'.join(segment)))
    assert segments[0] and all(s == segments[0] for s in segments), 'Segment definitions diverged'
    rays = [line for line in lines if line.startswith(TRACE)]
    assert len(rays) == 5, 'Expected four centered spotlight rays and one point ray'
    normalized = [re.sub(r'SpotSegment[0-3]', 'SpotSegmentN', line) for line in rays[:4]]
    assert len(set(normalized)) == 1, 'Trace parameters differ'
    assert '%SpotSegment0Max' in rays[0] and '%SpotSegment3Max' in rays[3]
    assert '%struct.ShadowHitData* nonnull %11)' in rays[0]
    # The pinned hit library has no global writes or stochastic state; payload is distance only.
    handlers = source[source.index('define void @'):source.index('define void @"\\01?spotRayGeneration')]
    assert not re.search(r'@dx\.op\.(?:bufferStore|rawBufferStore|textureStore|atomic)', handlers)
    edits = []
    for i in range(1, 4):
        store = f'  store float %SpotSegment{i}Max, float* %249, align 8\n'
        ray = rays[i]
        assert source.count(store) == 1
        # Leave the first hit in the payload; subsequent existing loads/aggregation
        # consume that same hit. No alias substitution or new floating-point math.
        before = store + ray
        assert source.count(before) == 1
        edits.append(before)
        source = source.replace(before, '')
    assert sum(line.startswith(TRACE) for line in source.splitlines()) == 2
    return source, edits


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--dxc', required=True)
    parser.add_argument('--assembler', required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    build = root / 'build'
    data = base64.b64decode((root/'assets/rt-spot-authored-clip/rt_spot_authored_clip.dxil.b64').read_text())
    assert hashlib.sha256(data).hexdigest() == BASE_SHA
    baseline = build/'centered-original.dxil';baseline.write_bytes(data)
    source = subprocess.check_output([args.dxc, '-dumpbin', str(baseline)], text=True)
    (build/'centered-original.ll').write_text(source)
    patched, edits = reuse_centered_rays(source)
    ir = build/'centered-reuse.ll';ir.write_text(patched)
    output = build/'rt_spot_centered_reuse.dxil'
    subprocess.run([args.assembler,str(ir),str(output),str(Path(args.dxc).parent/'dxcompiler.dll')],check=True)
    result = output.read_bytes()
    assert result[:4] == b'DXBC' and any(result[4:20]), 'Missing validated shader digest'
    check = subprocess.check_output([args.dxc,'-dumpbin',str(output)],text=True)
    assert sum(line.startswith(TRACE) for line in check.splitlines()) == 2
    # Replace only the existing authored-clip ray record. All compute shaders stay intact.
    (build/'rt_spot_authored_clip.h').write_text('#pragma once\nstatic const unsigned char kSpotAuthoredClipShader[]={\n'+','.join(f'0x{x:02x}' for x in result)+'\n};\n')
    report = dict(baseline_sha256=BASE_SHA,sha256=hashlib.sha256(result).hexdigest(),bytes=len(result),spot_traces_before=4,spot_traces_after=1,point_traces=1,removed_instructions=6,shader_validated=True)
    (build/'centered-reuse-validation.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report))

if __name__ == '__main__': main()
