"""Reject divergent rays and verify the transform only removes duplicate work."""
import sys
from pathlib import Path
from rt_spot_centered_reuse import reuse_centered_rays, TRACE
source=Path(sys.argv[1]).read_text()
patched,edits=reuse_centered_rays(source)
expected=source
for edit in edits:expected=expected.replace(edit,'',1)
assert patched==expected and len(edits)==3
assert sum(line.startswith(TRACE) for line in patched.splitlines())==2
for marker in ('%SpotSample1Hit = load','%SpotSample2Hit = load','%SpotSample3Hit = load','%SpotMeanBlocker = fdiv'):
 assert marker in patched
# Nonzero disk, different near-plane result, flags and payload layout must reject.
for before,after in [
 ('%SpotDisk2Radius, 0.000000e+00','%SpotDisk2Radius, 1.000000e+00'),
 ('%SpotSegment2Clipped = fmul float %SpotSegment2OriginalMax, %SpotSegment2Fraction','%SpotSegment2Clipped = fmul float %SpotSegment2OriginalMax, 1.000000e+00'),
 ('%struct.ShadowHitData = type { float }','%struct.ShadowHitData = type { float, i32 }'),
 ('float %SpotSegment2Max, %struct.ShadowHitData* nonnull %11)','float %SpotSegment2Max, %struct.ShadowHitData* nonnull %10)')]:
 assert before in source
 try:reuse_centered_rays(source.replace(before,after,1))
 except AssertionError:pass
 else:raise AssertionError('Unsafe shader accepted: '+before)
print('PASS: identical ray/segment contract, exact six-instruction removal, unchanged hit aggregation, divergent input rejection')
