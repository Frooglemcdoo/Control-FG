"""Audit recovered RR guides without attributing visual artifacts from ranges alone."""
import argparse
import hashlib
import json
from pathlib import Path
import numpy as np

def audit(path):
    metadata = json.loads((path / 'metadata.json').read_text())
    h, w = metadata['height'], metadata['width']
    hashes = {}
    def read(name, channels, dtype='<f2'):
        data = (path / name).read_bytes()
        if len(data) != h*w*channels*np.dtype(dtype).itemsize:
            raise ValueError(f'{name}: size does not match input extent')
        hashes[name] = hashlib.sha256(data).hexdigest()
        return np.frombuffer(data, dtype).reshape(h, w, channels).astype(np.float32)
    raw = read('raw-albedo.rgba16f', 4)
    normal = read('normal-roughness.rgba16f', 4)
    diffuse = read('diffuse.rgba16f', 4)
    specular = read('specular.rgba16f', 4)
    before = read('pre-filter-gi.rgba16f', 4)
    after = read('gi-at-rr-input.rgba16f', 4)
    responsivity = read('hair-responsivity.r16f', 1)[..., 0]
    g1 = read('native-gbuffer1.rgba8', 4, 'u1')
    valid = np.all((raw[..., :3] >= 0) & (raw[..., :3] <= 1), axis=2)
    hair = valid & (raw[..., 3] >= -3) & (raw[..., 3] <= -2)
    world = valid & (raw[..., 3] == -4)
    exact = valid & (~hair | (raw[..., 3] == -3))
    finite = {name: int(np.count_nonzero(~np.isfinite(a))) for name, a in
              [('normal_roughness', normal), ('diffuse_rgb', diffuse[..., :3]),
               ('specular_rgb', specular[..., :3]), ('gi_rgb', before[..., :3]),
               ('responsivity', responsivity)]}
    lengths = np.linalg.norm(normal[..., :3], axis=2)
    roughness_expected = (1 - g1[..., 2] / 255).astype(np.float16).astype(np.float32)
    return dict(frame=metadata['frame'], input_extent=[w, h],
        rr_active=metadata['rr_active'], reset=metadata['reset'],
        gi_boundary=metadata['gi_boundary'], hit_distance_bound=metadata['hit_distance_bound'],
        nonfinite=finite,
        normal_length_max_error=float(np.max(np.abs(lengths-1))),
        normal_length_error_over_002=int(np.count_nonzero(np.abs(lengths-1) > .02)),
        roughness_range=[float(normal[..., 3].min()), float(normal[..., 3].max())],
        roughness_max_error_vs_native_smoothness=float(np.max(np.abs(normal[..., 3]-roughness_expected))),
        exact_albedo_pixels=int(exact.sum()),
        exact_albedo_mismatch_pixels=int(np.count_nonzero(np.any(diffuse[..., :3] != raw[..., :3], axis=2) & exact)),
        valid_hair_pixels=int(hair.sum()), valid_world_pixels=int(world.sum()),
        fallback_pixels=int((~valid).sum()),
        responsivity_nonzero_outside_valid_hair=int(np.count_nonzero((responsivity != 0) & ~hair)),
        responsivity_range=[float(responsivity.min()), float(responsivity.max())],
        gi_rgb_equal=bool(np.array_equal(before[..., :3], after[..., :3])),
        gi_output_alpha_equals_red=bool(np.array_equal(after[..., 3], after[..., 0])),
        hashes=hashes)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('capture_root', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    paths = sorted(args.capture_root.glob('stationary-pair-*/*/metadata.json'))
    if not paths:
        parser.error('No recovered stationary metadata found')
    report = dict(schema='rr-guide-capture-audit-v1', frames=[audit(p.parent) for p in paths],
        limitations=['Recovered R271 captures, not R276 output images.',
            'Guide validity/range checks do not prove semantic correctness or temporal alignment.',
            'Hair/world tags identify replay coverage, not individual plant objects.',
            'Exact albedo check excludes partially covered hair, which intentionally blends guides.',
            'No quantitative claim about visible speckle reduction.'])
    args.output.write_text(json.dumps(report, indent=2, allow_nan=False)+'\n')
    for f in report['frames']:
        print({k:v for k,v in f.items() if k not in ('hashes','gi_boundary')})
