"""Audit pinned raygen sampling and analytic rejection bounds; no game simulation."""
import argparse
import hashlib
import json
import math
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

def constant(hex_value):
    return struct.unpack('>d', bytes.fromhex(hex_value))[0]

def audit():
    path = ROOT / 'research/native-integration-audit/reflection-000.ll'
    source = path.read_text()
    # Fail closed on the operations from which the mathematical bound follows.
    required = [
        '%270 = extractvalue %dx.types.CBufRet.i32 %269, 1',
        '%271 = mul i32 %270, %139',
        '%272 = add i32 %271, %reflectionRayIndex.029',
        '%.i1265 = fmul fast float %Frc64, 0x3FD99999A0000000',
        '%344 = fmul fast float %134, %134',
        '%345 = fmul fast float %344, %344',
        '%346 = fadd fast float %345, -1.000000e+00',
        '%275 = fsub fast float 1.000000e+00, %.i1265',
        '%276 = fmul fast float %.i1265, %346',
        '%277 = fadd fast float %276, 1.000000e+00',
        '%278 = fdiv fast float %275, %277',
        '%292 = fcmp fast olt float %291, 0x3F747AE140000000',
        '%296 = fmul fast float %295, 1.000000e+01',
        '%297 = fadd fast float %296, %293',
    ]
    for operation in required:
        assert source.count(operation) == 1, operation
    assert '@dx.op.cbufferLoadLegacy.i32(i32 59, %dx.types.Handle %sys_constants, i32 69)' in source
    limit = constant('3FD99999A0000000')
    reject = constant('3F747AE140000000')
    bounds = []
    for roughness in (.26, .278, .31762695, .356689, .36, .46):
        theta = math.atan(roughness**2 * math.sqrt(limit/(1-limit)))
        critical = math.acos(reject) - 2*theta
        # Independent direct-vector reflection check at the limiting azimuth.
        for angle in (0., 30., 60., 75., 85.):
            a = math.radians(angle)
            h = (-math.sin(theta), 0., math.cos(theta))
            v = (math.sin(a), 0., math.cos(a))
            ndotl = 2*sum(x*y for x,y in zip(h,v))*h[2]-v[2]
            assert abs(ndotl-math.cos(a+2*theta)) < 1e-12
        bounds.append(dict(roughness=roughness,
                           maximum_half_vector_angle_degrees=math.degrees(theta),
                           rejection_excluded_below_view_angle_degrees=math.degrees(critical)))
    # Conditional support bounds, NOT observations of the game's temporal counter.
    coverage = [dict(counter_period=p, spec=s, maximum_primary_indices_per_cycle=p*s)
                for p in (1, 8, 16, 32, 64) for s in (2, 4, 8, 16)]
    return dict(source=str(path.relative_to(ROOT)), source_sha256=hashlib.sha256(path.read_bytes()).hexdigest(),
                temporal_counter_byte_offset=1108, primary_index='temporal_counter * sample_count + sample_index',
                retry_index='temporal_counter + 10 * retry_number', radial_cdf_limit=limit,
                rejection_ndotl_threshold=reject, angular_bounds=bounds, conditional_coverage=coverage,
                actual_counter_period_observed=False, actual_view_angles_observed=False,
                game_quality_verified=False, runtime_changed=False,
                limitations=['Analytic real-arithmetic bounds; allow float error near boundary.',
                             'Index support is not independent ray count or quality.',
                             'A changing world-space noise texel can change directions even with repeating indices.',
                             'Captured roughness values are prior analysis inputs, not remeasured by this script.'])

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    result = audit()
    args.output.write_text(json.dumps(result, indent=2)+'\n')
    print(json.dumps(result['angular_bounds'], indent=2))
