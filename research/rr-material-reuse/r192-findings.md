# R192 observed temporal counter

Input: Lighting-R192-20261028-064335-22424-0.log and
image(20261028-114600).png, supplied by the user.

The analyzer reports 1024 consecutive valid CPU-provider observations, engine
frames 3178 through 4201, with no gaps, invalid reads or camera-cut flags.
The provider repeats 3,0,1,2 throughout: period four, four distinct values.
The interval spans 15.750 seconds (log ms 357872109 through 357887859).
All observations report effective SPEC4. Nearby heartbeats at presents 3238,
3581 and 3906 report native_pcf_blend, RR selected, last_result=1. The screenshot
shows NATIVE BLEND selected and SPEC:4, with mottled painting shading still
visible. The RR OFF button is an action, not evidence RR was disabled.

The known ray library replacement was created successfully at present 2471;
its retained R191 trace label is expected in this R192 diagnostic. This does
not establish the GPU value of sys_constants[69].y on the sampled frames.

The observed period supports the short-cycle hypothesis at the CPU provider.
Combined with the audited index formula counter*SPEC+sample, a GPU counter
with the same cycle would expose primary indices 0..15 at SPEC4, 0..31 at
SPEC8, and 0..63 at SPEC16. Those are index support counts across four frames,
not independent ray counts, quality equivalences, or fresh samples accumulated
indefinitely. World-space noise texel changes can also change directions.

This is a concrete explanation candidate for increased stability at SPEC16,
not a proven root cause. Next establish the producer/GPU connection or perform
a clearly labeled causal sequence experiment preserving SPEC, mapping, weights,
lighting and clamp. Do not globally overwrite the shared temporal provider:
other renderer consumers may depend on its four-frame range. A future change
should be confined to the exact reflection ray library and have bounded phases.
No R185 phase-precision retest: this finding concerns period, not float error.

No additional R192 capture is needed. No quality/performance improvement is
claimed by this diagnostic result.
