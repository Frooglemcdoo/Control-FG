"""Current-frame GI excess redistribution reference for the R438 candidate.

Every donor owns its normalization. Receiver-normalized smoothing is not an
equivalent operation and does not conserve RGB near asymmetric boundaries.
Inputs must be from the same frame and sampling grid. Coverage is an explicit
adapter responsibility; material IDs are only compared within this frame.
"""
from dataclasses import dataclass
import numpy as np

LUMA = np.array([.2126, .7152, .0722])


@dataclass(frozen=True)
class Settings:
    radius: int = 2
    threshold: float = .1  # Recorded lamp diagnostic, NOT a universal default.
    normal_cosine: float = .95
    relative_depth: float = .01


def redistribute(rgb, depth, normals, material, coverage, settings=Settings()):
    """Return candidate, donor count; retain invalid pixels without spreading them.

    Coverage is a discrete, caller-validated class, with zero meaning unknown.
    No history, ray direction, ray count, material response or light is changed.
    """
    rgb = np.asarray(rgb, dtype=np.float64)
    depth, normals, material, coverage = map(np.asarray, (depth, normals, material, coverage))
    if rgb.ndim != 3 or rgb.shape[-1] != 3:
        raise ValueError('Expected H x W x 3 radiance')
    shape = rgb.shape[:2]
    if any(a.shape != shape for a in (depth, material, coverage)) or normals.shape != rgb.shape:
        raise ValueError('All guides must match the radiance grid')
    if not 1 <= settings.radius <= 4 or not np.isfinite(settings.threshold) or settings.threshold < 0:
        raise ValueError('Invalid footprint or threshold')
    if not 0 <= settings.normal_cosine <= 1 or not 0 <= settings.relative_depth <= 1:
        raise ValueError('Invalid surface bounds')
    valid = (np.isfinite(rgb).all(-1) & (rgb >= 0).all(-1) & np.isfinite(depth) & (depth > 0)
             & np.isfinite(normals).all(-1) & (abs(np.linalg.norm(normals, axis=-1)-1) < 1e-3)
             & (coverage > 0))
    lum = rgb @ LUMA
    # Reads are immutable: processing order must never change donor selection,
    # median, compatibility or normalization.
    result = rgb.copy()
    count = 0
    h, w = shape
    r = settings.radius
    for y, x in zip(*np.where(valid & (lum > settings.threshold))):
        ys, xs = slice(max(0, y-r), min(h, y+r+1)), slice(max(0, x-r), min(w, x+r+1))
        eligible = (valid[ys, xs] & (material[ys, xs] == material[y, x])
                    & (coverage[ys, xs] == coverage[y, x])
                    & (normals[ys, xs] @ normals[y, x] >= settings.normal_cosine)
                    & (abs(depth[ys, xs]-depth[y, x]) <= settings.relative_depth*depth[y, x]))
        n = int(eligible.sum())
        if n <= 1:
            continue
        floor = float(np.median(lum[ys, xs][eligible]))
        if floor >= lum[y, x]:
            continue
        excess = rgb[y, x] * (1-floor/lum[y, x])
        result[y, x] -= excess
        result[ys, xs] += eligible[..., None] * (excess/n)
        count += 1
    return result, count
