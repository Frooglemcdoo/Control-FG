# R75: support both native spotlight ray libraries

The R74 log loaded all four compute components, but used native ray library
`d5efe2c2219257e2a7b51bbd65a7fa3b80cdb56fb7e6444223f8b4d7ee45a1a9`.
The runtime only augmented the single-ray library `3c6bee7e...`, so no compatible
replacement ray record was prepared and the mode was correctly unavailable.

R75 recognizes both exact native hashes and adds a matching ray-generation export
to each state object. The new `rt_spot_multi.dxil` retains the configurable native
sample loop, point rays, alpha handlers, resource layout and hit/miss tables.
Shared segment-generation code applies bounded per-sample endpoints, fixed emitter
radius and closest-blocker traversal to its spotlight rays. Its native accumulation
already excludes misses from nearest-blocker reduction. The single-ray replacement
retains its four symmetric samples. Unknown native shader libraries remain native.

All seven shader files must be installed with the DLL while Control is closed.
Enable native RT Contact Shadows, load the affected room, then select RT SPOTLIGHT
REPLACEMENT through F10. No restart is needed between modes. SAVE LOG records R75
and the recognized ray variant (`single` or `configurable`).

The R74 current-frame spotlight history test is retained. Noise may increase;
this build repairs variant support, not the unconfirmed visual issues. Check wall
flicker, hallway pop-in and Jesse's incomplete near-light shadow separately.

Validation: the generated configurable library passes DXIL validation; local
captured-IR checks retain native point code, handlers and sample-loop bounds.
Package checks verify every asset hash against the manifest and runtime loader.
Windows CI validates compute pipelines, switching, logging and the DLL build.
DXR state-object linking and visual behavior still require the target GPU test.

Reproduce the additional shader using `tools/generate-rt-spot-pair.py` with the
R65 compute capture directory and `--multi-ray-capture` pointing to the R64 capture
directory, plus the existing DXC, assembler and DXV arguments.
