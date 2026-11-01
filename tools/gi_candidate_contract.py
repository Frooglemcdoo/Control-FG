"""Offline same-pixel GI accounting reference, NOT a spatial ReSTIR implementation.

Native throughput already includes the sampling cancellation. These contributions
must not be divided by a directional PDF again. A source-pixel contribution is
not a portable secondary radiance and cannot be evaluated at another pixel.
"""
from dataclasses import dataclass
import math


@dataclass(frozen=True)
class Ray:
    index: int
    material: int
    throughput: tuple
    secondary_radiance: tuple

    def contribution(self):
        return tuple(a*b for a,b in zip(self.throughput,self.secondary_radiance))


@dataclass(frozen=True)
class LocalChoice:
    source_pixel: tuple
    probability: float
    surface_estimate: tuple
    fallback: tuple

    def evaluate(self, pixel):
        if pixel != self.source_pixel:
            raise ValueError('Cross-pixel reuse needs a target PDF, visibility and directional reevaluation')
        return tuple(a+b for a,b in zip(self.surface_estimate,self.fallback))


def choices(pixel, rays, configured_count, depth_scale):
    """Possible outcomes of weighted local selection, retaining native misses.

Returns exact probabilities, allowing deterministic expectation tests instead
of Monte Carlo tolerances. This selection cannot discover an unsampled light
path and is not a variance-reduction claim.
"""
    if configured_count < 1 or len(rays) != configured_count or {r.index for r in rays} != set(range(configured_count)):
        raise ValueError('Missing or duplicate native rays')
    if not math.isfinite(depth_scale) or depth_scale < 0:
        raise ValueError('Invalid depth scale')
    for ray in rays:
        if ray.material < 0 or ray.material > 65535 or ray.material == 65534:
            raise ValueError('Invalid material sentinel')
        if len(ray.throughput) != 3 or len(ray.secondary_radiance) != 3:
            raise ValueError('RGB required')
        if any(not math.isfinite(v) or v < 0 for v in ray.throughput+ray.secondary_radiance):
            raise ValueError('Nonfinite or negative native lighting')
    # Keep N, not number of hits: changing the denominator brightens the image.
    scale = depth_scale/configured_count
    fallback = tuple(scale*math.fsum(r.contribution()[c] for r in rays if r.material == 65535) for c in range(3))
    candidates = []
    for ray in rays:
        if ray.material == 65535:
            continue
        value = ray.contribution()
        weight = math.fsum(a*b for a,b in zip(value,(.2126,.7152,.0722)))
        if weight > 0:
            candidates.append((value,weight))
    weight_sum = math.fsum(weight for value,weight in candidates)
    if not weight_sum:
        return [LocalChoice(pixel,1.,(0.,0.,0.),fallback)]
    return [LocalChoice(pixel,weight/weight_sum,
                        tuple(scale*v*weight_sum/weight for v in value),fallback)
            for value,weight in candidates]


def expectation(pixel, outcomes):
    return tuple(math.fsum(o.probability*o.evaluate(pixel)[c] for o in outcomes) for c in range(3))
