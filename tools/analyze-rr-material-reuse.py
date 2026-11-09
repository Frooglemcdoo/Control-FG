"""Offline feasibility probe, NOT a runtime shader or RR quality benchmark.

Usage: python tools/analyze-rr-material-reuse.py CAPTURE_ROOT --output report.json
Requires numpy/scipy. Uses linear captured lighting; never filters RR output.
"""
import argparse
import hashlib
import json
from pathlib import Path

import numpy as np
from scipy.ndimage import gaussian_filter

LUMA = np.array([.2126, .7152, .0722])
ROIS = {'painting': (1200, 600, 1460, 800), 'elevator': (900, 650, 1300, 790)}


def load(folder, role, roi):
    meta = json.loads((folder / 'metadata.json').read_text())
    r = next(r for r in meta['resources'] if r['file'].startswith(role + '.'))
    assert r['copied'] and meta['row_padding_removed'] and meta['little_endian']
    dtype = '<f4' if r['file'].endswith('r32f') else '<f2'
    channels = r['pixel_bytes'] // np.dtype(dtype).itemsize
    path = folder / r['file']
    assert path.stat().st_size == r['height'] * r['width'] * r['pixel_bytes']
    a = np.memmap(path, dtype=dtype, mode='r', shape=(r['height'], r['width'], channels))
    x0, y0, x1, y1 = roi
    result = np.array(a[y0:y1, x0:x1], dtype=np.float64)
    assert np.isfinite(result).all()
    return result


def reuse(rgb, guides, radius):
    """Symmetric pair exchange: conserves each RGB sum, positive weights.

    Weights depend on surface guides, never noisy radiance. A fixed upper
    bound on total weights leaves a nonnegative center weight. Zero wrap.
    This conserves energy within the crop but can still blur reflections
    within one material; primary-surface guides cannot detect those edges.
    """
    n, rough, albedo, depth = guides
    output = rgb.copy()
    # Include the center in the budget: retain at least 1/window_area of it.
    # Without this, a flat guide field turns radius 1 into a ring filter that
    # can erase an isolated center and spread its energy around it.
    budget = float((2 * radius + 1) ** 2)
    h, w = rgb.shape[:2]
    for dy in range(radius + 1):
        for dx in range(-radius, radius + 1):
            if dy == 0 and dx <= 0:
                continue
            a = (slice(0, h-dy), slice(max(0, -dx), min(w, w-dx)))
            b = (slice(dy, h), slice(max(0, dx), min(w, w+dx)))
            normal_delta = np.sum((n[a]-n[b])**2, axis=2)
            rough_delta = ((rough[a]-rough[b])/.04)**2
            albedo_delta = np.sum(((albedo[a]-albedo[b])/.08)**2, axis=2)
            # Device depth is used only as an explicit heuristic. This is
            # not a metric view-space distance or a runtime rejection rule.
            depth_delta = ((depth[a]-depth[b])/1e-5)**2
            weight = np.exp(-normal_delta/.02-rough_delta-albedo_delta-depth_delta)/budget
            exchange = weight[..., None] * (rgb[b]-rgb[a])
            output[a] += exchange
            output[b] -= exchange
    assert np.allclose(output.sum((0, 1)), rgb.sum((0, 1)), rtol=1e-12, atol=1e-12)
    assert output.min() >= min(0, rgb.min()) - 1e-12
    return output


def metrics(pair):
    a, b = [x @ LUMA for x in pair]
    mean = float(a.mean())
    return dict(mean=mean, high_frequency_rms_over_mean=float((a-gaussian_filter(a, 2)).std()/mean),
                frame_difference_rms_over_mean=float(np.sqrt(np.mean((a-b)**2))/mean),
                peak_over_mean=float(a.max()/mean))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('captures', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    report = {'runtime_changed': False, 'rr_evaluated': False,
              'limitations': ['Two jittered frames: difference includes sampling and subpixel scene changes.',
                              'No SPEC16 reference; reduced high frequencies do not establish quality.',
                              'Primary guides cannot protect reflection detail inside one surface.',
                              'Captured specular is downstream of existing filtering; pre-clamp behavior is untested.',
                              'No GPU performance measurement.'], 'scenes': {}}
    for scene, roi in ROIS.items():
        folders = sorted(p for p in (args.captures/scene).iterdir() if (p/'metadata.json').is_file())
        assert len(folders) == 2
        raw, candidates = [], {1: [], 2: []}
        hashes = {}
        for folder in folders:
            rgb = load(folder, 'lighting-specular', roi)[..., :3]
            nr = load(folder, 'normal-roughness', roi)
            normals = nr[..., :3] / np.maximum(np.linalg.norm(nr[..., :3], axis=2, keepdims=True), 1e-9)
            guides = (normals, nr[..., 3], load(folder, 'specular', roi)[..., :3],
                      load(folder, 'input-depth', roi)[..., 0])
            raw.append(rgb)
            for radius in candidates:
                candidates[radius].append(reuse(rgb, guides, radius))
            hashes[folder.name] = hashlib.sha256((folder/'metadata.json').read_bytes()).hexdigest()
        report['scenes'][scene] = {'roi_xyxy': roi, 'metadata_sha256': hashes, 'original': metrics(raw),
                                  'candidates': {str(r): metrics(pair) for r, pair in candidates.items()}}
    args.output.write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
