"""Compare primary native sequence with stratification in sample space only.

Does not model GGX mapping, rejection retries, visibility, shading or RR.
The random shifts approximate different blue-noise texels; they are not a
reconstruction of Control's actual texture or evidence of in-game quality.
"""
import argparse
import json
import struct
from pathlib import Path
import numpy as np


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output', type=Path, required=True)
    args = p.parse_args()
    root = Path(__file__).resolve().parents[1]
    source = (root/'research/native-integration-audit/reflection-000.ll').read_text()
    constants = ('3FE827F540000000', '3FE23C21C0000000')
    for value in constants:
        assert '0x'+value in source
    alpha = np.array([struct.unpack('>d', bytes.fromhex(x))[0] for x in constants], dtype=np.float32)
    rng = np.random.default_rng(181)
    trials = 131072
    frames = rng.integers(1, 32768, (trials, 1), dtype=np.uint32)
    shifts = rng.random((trials, 1, 2)).astype(np.float32)
    candidates = {}
    for count in (2, 4, 8, 16):
        indices = frames*count + np.arange(count, dtype=np.uint32)
        # Match the displayed native multiply/add float32 sequence. GPU
        # contraction/fast-math may differ, so this is not bitwise GPU proof.
        raw = indices.astype(np.float32)[..., None]*alpha + shifts
        candidates[f'native{count}'] = raw-np.floor(raw)
    # Jitter one point into each quadrant: a candidate, not the native shader.
    grid = np.array([[0,0], [1,0], [0,1], [1,1]])
    candidates['stratified4'] = (grid+rng.random((trials, 4, 2)))*.5
    tests = {
        'small_light': (lambda x: ((x[..., 0]-.5)**2+(x[..., 1]-.5)**2 < .05**2), np.pi*.05**2),
        'broad_light': (lambda x: ((x[..., 0]-.5)**2+(x[..., 1]-.5)**2 < .2**2), np.pi*.2**2),
        'vertical_strip': (lambda x: (np.abs(x[..., 0]-.5) < .025), .05),
        'horizontal_strip': (lambda x: (np.abs(x[..., 1]-.5) < .025), .05),
        'smooth_lobe': (lambda x: 1+np.cos(2*np.pi*x[..., 0])*np.cos(2*np.pi*x[..., 1]), 1),
    }
    result = {'trials': trials, 'seed': 181, 'domain': 'unit-square sampling coordinates, not scene radiance',
              'runtime_changed': False, 'cases': {}}
    for name, (evaluate, expected) in tests.items():
        result['cases'][name] = {}
        for sampler, points in candidates.items():
            estimates = evaluate(points).mean(axis=1)
            result['cases'][name][sampler] = {
                'relative_rmse': float(np.sqrt(np.mean((estimates-expected)**2))/expected),
                'relative_mean_error': float(estimates.mean()/expected-1),
                'zero_hit_fraction': float(np.mean(estimates == 0))}
    args.output.write_text(json.dumps(result, indent=2)+'\n')
    for case, values in result['cases'].items():
        print(case, ' '.join(f'{k}:RMSE={v["relative_rmse"]:.4f}' for k,v in values.items()))


if __name__ == '__main__':
    main()
