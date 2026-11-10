"""Exact counterexample for retired endpoint reuse, plus production reachability guard.
This is an estimator fixture, not a replay of captured game pixels or RR.
"""
from fractions import Fraction as F
from itertools import product
from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
shader = (root / "src/shaders/gi_temporal_hit.hlsl").read_text()
# Keep the counterexample tied to the retired shader's actual distance gates.
cap = float(re.search(r"lengthToHit>([0-9.]+)f", shader).group(1))
assert cap == 65
assert "lengthToHit>asfloat(controls.x)*.8f" in shader
assert "((p.x^p.y^frame)&1)!=0" in shader
assert "materials[uint3(p,priorSlice)]>=65534" in shader
# Static receiver, same material, valid hit and visibility for both classes.
# At range 100, distances 10 and 70 are both below even the 80-unit
# minimum fresh-ray range. Only the 65-unit reuse gate distinguishes them.
distances = (10, 70)
assert all(.03 < d < 100 * .8 for d in distances)

def mean(rays, reuse, bright_near=True, accept_both=False):
    total = F(0)
    # Equal-probability fresh/prior classes, both receiver parity phases.
    for fresh, prior, parity in product(range(2), repeat=3):
        chosen = fresh
        if reuse and parity == 0 and (accept_both or distances[prior] <= cap):
            chosen = prior
        value = int((chosen == 0) == bright_near)
        # One replaced slice; other slices retain their exact expectation.
        total += (F(value) + F(rays - 1, 2)) / rays / 8
    return total

for rays in (1, 2, 4, 16, 64):
    for bright in (True, False):
        assert mean(rays, False, bright) == F(1, 2)
        expected = F(1, 2) + (1 if bright else -1) * F(1, 8 * rays)
        assert mean(rays, True, bright) == expected
        assert mean(rays, True, bright, True) == F(1, 2)
print("PASS: exact distance-selection counterexample; 1 ray mean .5 -> .625/.375; fresh .5")

# Prevent reintroducing the retired pass through either production entry point.
# The old implementation stays in source for the reproducible audit above.
live = (root / "src/native_lighting_live_runtime.h").read_text()
toggle = (root / "src/gi_live_toggle.h").read_text()
assert "gi_temporal::" not in live
assert 'include "gi_temporal_runtime.h"' not in live
assert "temporalRecord" not in toggle
assert "gi_live_toggle::Create(&rt_spot_runtime::CreateAugmented" in live
assert "rt_shadow_capture::Lookup(list).dispatch(list,&custom)" in toggle
assert "gi_reuse::singleEvaluatorReady.load()" in toggle
print("PASS: production uses fresh Owned rays without temporal state augmentation or dispatch")
