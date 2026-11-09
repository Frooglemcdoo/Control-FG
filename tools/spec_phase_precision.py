"""Keep native R2 sample phases bounded before adding the blue-noise offsets.

The pinned float32 constants are exact multiples of 2^-24. Integer modular
products recover their fractional phases without converting a growing index
to float. The native GGX mapping, truncation, retries and lighting remain.
"""
import argparse
import hashlib
import json
import struct
import subprocess
from pathlib import Path
from spec_retry_reuse import ROOT, SOURCE_SHA, transform as reuse

CONSTANTS = ('3FE827F540000000', '3FE23C21C0000000', '3FF9E377A0000000')
NUMERATORS = tuple(int(struct.unpack('>d', bytes.fromhex(h))[0] * 2**24) for h in CONSTANTS)


def edits():
    result = []
    for dim, constant, numerator in zip(range(3), CONSTANTS, NUMERATORS):
        dst = ('%.i0255', '%.i1256', '%.i2257')[dim]
        old = f'  {dst} = fmul fast float %273, 0x{constant}\n'
        new = (f'  %PhaseProduct{dim} = mul i32 %272, {numerator}\n'
               f'  %PhaseBits{dim} = and i32 %PhaseProduct{dim}, 16777215\n'
               f'  %PhaseFloat{dim} = uitofp i32 %PhaseBits{dim} to float\n'
               f'  {dst} = fmul float %PhaseFloat{dim}, 0x3E70000000000000\n')
        result.append((old, new))
    return result


def transform(source):
    original = reuse(source)
    result = original
    for old, new in edits():
        assert result.count(old) == 1, 'Native primary phase changed'
        result = result.replace(old, new)
    # This conversion is now unused. Keep numeric SSA numbering intact for LLVM.
    # DXC's optimizer may remove it after validation; no runtime use remains.
    restored = result
    for old, new in edits():
        restored = restored.replace(new, old)
    assert restored == original
    return result


def fixture():
    # Matching compute arithmetic for WARP testing against a 64-bit CPU oracle.
    # Actual production IR is independently interpreted by the Python test.
    terms = ','.join(str(n)+'u' for n in NUMERATORS)
    return f'''StructuredBuffer<uint4> Jobs : register(t0);
RWStructuredBuffer<float4> Result : register(u0);
cbuffer Settings : register(b0) {{ uint count; }}
[numthreads(64,1,1)]
void main(uint3 id : SV_DispatchThreadID) {{
    if (id.x >= count) return;
    uint4 job = Jobs[id.x];
    uint3 bits = (job.x * uint3({terms})) & 0xFFFFFFu;
    precise float3 phase = float3(bits) * (1.0 / 16777216.0);
    precise float3 shifted = phase + asfloat(job.yzw);
    Result[id.x] = float4(frac(shifted), 1);
}}
'''


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--dxc', required=True)
    p.add_argument('--assembler', required=True)
    args = p.parse_args()
    build = ROOT/'build'; build.mkdir(exist_ok=True)
    native = ROOT/'research/hit-distance/evidence/original-shaders/rt_reflection-000.dxbc'
    assert hashlib.sha256(native.read_bytes()).hexdigest() == SOURCE_SHA
    source = subprocess.check_output([args.dxc, '-dumpbin', str(native)], text=True)
    ir = build/'spec-phase-precision.ll'; ir.write_text(transform(source))
    output = build/'spec-phase-precision.dxil'
    subprocess.run([args.assembler, str(ir), str(output), str(Path(args.dxc).parent/'dxcompiler.dll')], check=True)
    data = output.read_bytes()
    assert data[:4] == b'DXBC' and any(data[4:20])
    (build/'spec_phase_precision.generated.h').write_text('#pragma once\nstatic const unsigned char kSpecPhasePrecisionShader[]={\n'+','.join(f'0x{x:02x}' for x in data)+'\n};\n')
    shader = build/'spec-phase-fixture.hlsl'; shader.write_text(fixture())
    subprocess.run([args.dxc, '-T', 'cs_6_0', '-E', 'main', '-O3', '-Fo', str(build/'spec-phase-fixture.dxil'), str(shader)], check=True)
    report = dict(source_sha256=SOURCE_SHA, sha256=hashlib.sha256(data).hexdigest(), bytes=len(data),
                  variant=0, phase_numerators=NUMERATORS, fractional_bits=24,
                  original_sequence_constants=True, original_mapping=True,
                  sample_count_changed=False, lighting_weights_changed=False,
                  retry_cache_retained=True, dxil_validated=True,
                  hardware_performance_measured=False, game_quality_verified=False)
    (build/'spec-phase-validation.json').write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps(report))


if __name__ == '__main__':
    main()
