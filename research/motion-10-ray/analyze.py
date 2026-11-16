"""Read-only audit of the user-confirmed 10-ray motion pair.

Rectangles are manually inspected skin regions, not object segmentation.
Output footprints use resolution scaling, not exact temporal correspondence.
Local contrast is a diagnostic, not a measure of correct lighting.
"""
import json
import sys
from pathlib import Path

import numpy as np
from scipy.ndimage import median_filter

root = Path(sys.argv[1])
regions = {'jaw_neck': (440, 710, 560, 800), 'hand': (945, 890, 1010, 970)}
report = {'ray_count': 10, 'ray_count_evidence': 'user confirmation; absent from capture metadata',
          'regions': regions, 'frames': []}
for folder in sorted(root.glob('rr-input*')):
    meta = json.loads((folder / 'metadata.json').read_text())
    def read(name):
        r = next(r for r in meta['resources'] if r['file'].split('.')[0] == name)
        dtype = '<f4' if r['file'].endswith('r32f') else '<f2'
        return np.memmap(folder / r['file'], dtype=dtype, mode='r').reshape(r['height'], r['width'], -1)
    names = ['pre-filter-gi', 'gi-at-rr-input', 'input-color', 'rr-output',
             'lighting-diffuse', 'lighting-specular', 'diffuse', 'specular',
             'normal-roughness', 'raw-albedo', 'input-motion', 'input-depth']
    data = {n: read(n) for n in names}
    frame = {'frame': meta['frame'], 'rr_active': meta['rr_active'], 'reset': meta['reset'],
             'consecutive': meta.get('consecutive'), 'gi_boundary': meta['gi_boundary']['stage'],
             'gi_rgb_exact_match': bool(np.array_equal(data['pre-filter-gi'][..., :3], data['gi-at-rr-input'][..., :3])),
             'nonfinite_values': {n: int(np.count_nonzero(~np.isfinite(a))) for n, a in data.items()},
             'regions': {}}
    for label, (x, y, X, Y) in regions.items():
        a = {n: v[y:Y, x:X].astype(np.float64) for n, v in data.items() if n != 'rr-output'}
        residual = a['input-color'][..., 0] - median_filter(a['input-color'][..., 0], size=5)
        # Restrict peak selection to the captured positive skin tag.
        valid = a['raw-albedo'][..., 3] == 1
        order = np.argsort(np.where(valid, residual, -np.inf).ravel())[::-1]
        chosen = []
        for idx in order:
            py, px = np.unravel_index(idx, residual.shape)
            if not valid[py, px] or residual[py, px] <= 0:
                break
            if all((px-u)**2+(py-v)**2 >= 36 for v, u in chosen):
                chosen.append((int(py), int(px)))
            if len(chosen) == 8:
                break
        gi_lum = a['pre-filter-gi'][..., :3] @ [.2126, .7152, .0722]
        gi_med = median_filter(gi_lum, size=5)
        peaks = []
        for py, px in chosen:
            ix, iy = x+px, y+py
            out = data['rr-output']
            ox = int((ix+.5)*out.shape[1]/meta['width'])
            oy = int((iy+.5)*out.shape[0]/meta['height'])
            patch = out[oy-4:oy+5, ox-4:ox+5, :3].astype(float)
            center = patch[3:6, 3:6, 0]
            p = {'input_xy': [ix, iy], 'raw_gi_luminance_over_5x5_median': float(gi_lum[py, px]/max(gi_med[py, px], 1e-12)),
                 'input_rgb': a['input-color'][py, px, :3].tolist(),
                 'diffuse_reconstructed_fraction_rgb': (a['lighting-diffuse'][py, px, :3]*a['diffuse'][py, px, :3]/np.maximum(a['input-color'][py, px, :3], 1e-12)).tolist(),
                 'rr_scaled_footprint_xy': [ox, oy],
                 'rr_center_max_red_over_9x9_median': float(center.max()/max(np.median(patch[..., 0]), 1e-12)),
                 'normal_length': float(np.linalg.norm(a['normal-roughness'][py, px, :3])),
                 'roughness': float(a['normal-roughness'][py, px, 3]),
                 'motion_pixels': (a['input-motion'][py, px]*meta['motion_scale']).tolist()}
            peaks.append(p)
        frame['regions'][label] = {'peaks': peaks}
    report['frames'].append(frame)
print(json.dumps(report, indent=2))
