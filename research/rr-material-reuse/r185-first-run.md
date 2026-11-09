# R185 first run

User reports no visible difference. Input: Lighting-R185-20261027-152937-39792-0.log.

The matching ray library was successfully created at present 4915:
`SPEC_PHASE_PRECISION build=R185 applied=1 native_fallback=0`.

This log does not establish the requested RR-on SPEC4 comparison. Every logged
specular request is 0 (native). The producer reports native=1 effective=1,
including frame 5040 with RR selected. RR option snapshots show selected=1 at
frames 4980 and 5040, then selected=0 at frame 5113 through the final snapshots
(frame 7680). These snapshots do not give exact toggle timestamps.

Accept the user's observation as no visible improvement in this run. Do not
describe it as a confirmed SPEC4/RR-on rejection, a failed shader installation,
or evidence of performance gain/loss. No performance measurement was supplied.
The GPU temporal counter remains unobserved in this log.

User clarification: the uploaded log was a separate retest. The FIRST visual
test used SPEC4 and showed no visible difference. Accept R185 as no visible
improvement at SPEC4 based on that report; do not request another repetition.
R186 excludes its precision change and uses R184 as the rendering baseline.
