"""R179: decorrelate rejected SPEC 2/4 directions; retain R176 for other counts.

Only the uniform variates of rejected low-count samples change. The native
truncated GGX mapping, acceptance test, retry cap and all tracing/shading stay.
This is a variance experiment, not a claim of SPEC 16 quality at four rays.
"""
import argparse
import hashlib
import json
import subprocess
from pathlib import Path
from spec_retry_reuse import ROOT, SOURCE_SHA, transform as reuse


def mix_ir(prefix, value):
    # Integer avalanche; uint overflow is intentional (no nuw/nsw flags).
    return '\n'.join([
        f'  %{prefix}0 = lshr i32 {value}, 16',
        f'  %{prefix}1 = xor i32 {value}, %{prefix}0',
        f'  %{prefix}2 = mul i32 %{prefix}1, 2146121005',
        f'  %{prefix}3 = lshr i32 %{prefix}2, 15',
        f'  %{prefix}4 = xor i32 %{prefix}2, %{prefix}3',
        f'  %{prefix}5 = mul i32 %{prefix}4, 2221713035',
        f'  %{prefix}6 = lshr i32 %{prefix}5, 16',
        f'  %{prefix}7 = xor i32 %{prefix}5, %{prefix}6',
        f'  %{prefix}8 = lshr i32 %{prefix}7, 8',
        f'  %{prefix}9 = uitofp i32 %{prefix}8 to float',
        f'  %{prefix}Uniform = fmul float %{prefix}9, 0x3E70000000000000',
    ]) + '\n'


def transform(source):
    result = reuse(source)
    def replace(before, after):
        nonlocal result
        assert result.count(before) == 1, before
        result = result.replace(before, after, 1)
    replace('  %268 = icmp sgt i32 %139, 0',
            '  %LowSpec2 = icmp eq i32 %139, 2\n'
            '  %LowSpec4 = icmp eq i32 %139, 4\n'
            '  %LowSpec = or i1 %LowSpec2, %LowSpec4\n'
            '  %NativeSpec = xor i1 %LowSpec, true\n'
            '  %268 = icmp sgt i32 %139, 0')
    replace('RetryGate:\n  br i1 %RetryCached,',
            'RetryGate:\n  %RetryCanReuse = and i1 %RetryCached, %NativeSpec\n  br i1 %RetryCanReuse,')
    first = '  %.i0294 = fmul fast float %297, 0x3FE827F540000000'
    last = '  %Frc47 = call float @dx.op.unary.f32(i32 22, float %.i1297)  ; Frc(value)\n'
    a = result.index(first); b = result.index(last, a) + len(last)
    native = result[a:b]
    assert native.count('\n') == 6
    renamed = native.replace('%Frc47 =', '%NativeRetryV =').replace('%Frc =', '%NativeRetryU =')
    patch = ('  br i1 %LowSpec, label %LowRetry, label %NativeRetry\n\n'
             'NativeRetry:\n' + renamed + '  br label %LowRetryMerge\n\n'
             'LowRetry:\n'
             '  %LowSampleSeed = mul i32 %272, 2654435769\n'
             '  %LowNoiseU = bitcast float %.i0258 to i32\n'
             '  %LowNoiseV = bitcast float %.i1260 to i32\n'
             '  %LowNoiseMix = mul i32 %LowNoiseV, 668265261\n'
             '  %LowNoiseSeed = xor i32 %LowNoiseU, %LowNoiseMix\n'
             '  %LowWorldSeed = xor i32 %LowNoiseSeed, %257\n'
             '  %LowPixelSeed = xor i32 %LowSampleSeed, %LowWorldSeed\n'
             '  %LowAttemptSeed = mul i32 %294, 2246822507\n'
             '  %LowSeed = xor i32 %LowPixelSeed, %LowAttemptSeed\n'
             '  %LowSeedV = xor i32 %LowSeed, 1757159915\n' +
             mix_ir('LowU', '%LowSeed') + mix_ir('LowV', '%LowSeedV') +
             '  br label %LowRetryMerge\n\n'
             'LowRetryMerge:\n'
             '  %Frc = phi float [ %NativeRetryU, %NativeRetry ], [ %LowUUniform, %LowRetry ]\n'
             '  %Frc47 = phi float [ %NativeRetryV, %NativeRetry ], [ %LowVUniform, %LowRetry ]\n')
    replace(native, patch)
    # The original fallback's basis-selection PHIs now have this predecessor.
    start = result.index('"\\01?internal.getImportanceRay@@YA?AV?$vector@M$02@@V1@0MV?$vector@M$01@@@Z.exit13":')
    end = result.index('  %.i0303 =', start)
    block = result[start:end]
    assert block.count(', %.lr.ph ]') == 5
    replace(block, block.replace(', %.lr.ph ]', ', %LowRetryMerge ]'))
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--dxc', required=True)
    parser.add_argument('--assembler', required=True)
    args = parser.parse_args()
    build = ROOT/'build'; build.mkdir(exist_ok=True)
    native = ROOT/'research/hit-distance/evidence/original-shaders/rt_reflection-000.dxbc'
    assert hashlib.sha256(native.read_bytes()).hexdigest() == SOURCE_SHA
    source = subprocess.check_output([args.dxc, '-dumpbin', str(native)], text=True)
    ir = build/'spec-low-sample-retries.ll'; ir.write_text(transform(source))
    output = build/'spec-low-sample-retries.dxil'
    subprocess.run([args.assembler, str(ir), str(output), str(Path(args.dxc).parent/'dxcompiler.dll')], check=True)
    data = output.read_bytes()
    assert data[:4] == b'DXBC' and any(data[4:20])
    # Reuse the audited creation-only substitution and its descriptor tests.
    (build/'spec_retry_reuse.generated.h').write_text('#pragma once\nstatic const unsigned char kSpecRetryReuseShader[]={\n' + ','.join(f'0x{x:02x}' for x in data) + '\n};\n')
    report = dict(source_sha256=SOURCE_SHA, sha256=hashlib.sha256(data).hexdigest(), bytes=len(data),
                  experimental_counts=[2,4], other_counts='R176', ray_count_changed=False,
                  retry_cap=100, extra_gpu_resources=0, dxil_validated=True,
                  hardware_quality_measured=False, hardware_performance_measured=False)
    (build/'spec-low-sample-validation.json').write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps(report))


if __name__ == '__main__':
    main()
