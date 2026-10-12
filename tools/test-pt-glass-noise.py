"""Synthetic transport regression, not a substitute for in-game motion testing.

Compare the G2D1 and G2D2 scalar filter on noisy constant irradiance, isolated
high-energy paths, and sparse edge support. Native GI is deliberately used as
a biased low-noise prior; stable illumination must still change the output.
"""
import math
import random
import statistics

def estimate(values, weights, adaptive):
    total = sum(weights)
    mean = sum(v*w for v, w in zip(values, weights))/total
    blend = .5
    if adaptive:
        count = total*total/sum(w*w for w in weights)
        variance = max(sum(v*v*w for v, w in zip(values, weights))/total-mean*mean, 0)
        error = variance/max(count-1, 1)
        tolerance = .1*max(1, mean)
        blend *= tolerance*tolerance/(tolerance*tolerance+error)
        blend *= min(1, max(0, (count-4)/12))
    return 1+(mean-1)*blend

old_w = [2**(-(x*x+y*y)/8) for y in range(-3, 4) for x in range(-3, 4)]
new_w = [2**(-(x*x+y*y)/24) for y in range(-5, 6) for x in range(-5, 6)]
# Stable new illumination retains the original half blend, even far above GI.
for level in [0, 1, 2, 10]:
    assert math.isclose(estimate([level]*121, new_w, True), (1+level)/2, abs_tol=1e-10)
# A sliver with one eligible sample cannot inject unfiltered random light.
assert estimate([1000], [1], True) == 1
# A rare high-energy path is substantially attenuated.
v = [1.]*121
v[60] = 1000
old_v = [1.]*49
old_v[24] = 1000
assert estimate(v, new_w, True)-1 < .1*(estimate(old_v, old_w, False)-1)
rng = random.Random(403)
old, new = [], []
for _ in range(2000):
    tile = [[rng.expovariate(1) for x in range(11)] for y in range(11)]
    old.append(estimate([tile[y][x] for y in range(2, 9) for x in range(2, 9)], old_w, False))
    new.append(estimate([v for row in tile for v in row], new_w, True))
ratio = statistics.pstdev(new)/statistics.pstdev(old)
assert ratio < .5, ratio
assert abs(statistics.mean(new)-1) < .02
print(f'PASS: synthetic noise stddev ratio={ratio:.3f}; stable light retained; isolated outlier suppressed; sparse support falls back.')
