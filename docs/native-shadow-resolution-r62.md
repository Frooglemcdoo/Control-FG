# R62: native spotlight tile resolution experiment

R61 was rejected: its narrow PCF footprint hardened the shadows. R62 removes
that replacement from the runtime and retains native direct-light shader code.

Recovered PIX projects show active PCF depth-atlas viewports of 1024 square and
2048 square on High; corresponding Low passes use 512 and 1024. This establishes
per-pass sizes, not which light causes the screenshot's speckle.

The reviewed renderer exposes SpotLight dynamic resolution at object offset
0xD8. Native code at RVA 0x18AC77 multiplies it by the quality factor, then requests
a square tile through the PCF atlas virtual method at 0x18ACB6. The return address
0x18ACBA distinguishes these requests from sun and other atlas users. The PCF
allocator vtable is at 0x637400; resize is 0x1A19A0, request is 0x1A19E0.

The resize call returning to 0x1376C8 receives native atlas dimensions from the
quality table at 0x7EC288. R62 passes the same width and four times the height to
the original native method. The original engine handles texture creation,
packing, coordinates and resource ownership. Both modes retain this capacity;
only eligible dynamic spotlight requests double in the 2X mode. Original native
methods and all six allocation arguments are preserved. Unrecognized call sites,
non-square/invalid/oversized requests and unsupported dimensions pass through.
The known export and exact vtable targets/call instructions must match before
installation. VSM and precomputed static maps are outside this test.

At 16384x2048 R32 depth, expanded storage is 512 MiB rather than 128 MiB, before
allocation overhead. Native shading still has its original taps and jitter.
The experiment changes no shader radius; the visual softness still needs QA.
No restart is needed between mode selections. Installation requires relaunch.

Logs explicitly identify native/expanded atlas requests and native/doubled tile
requests. A returned request record is NOT evidence of final packing success or
GPU completion. Visual-result recording also requires the native direct shader
binding and a matching-sequence larger request. Capacity pressure, cached maps,
scene transitions, retained softness and actual noise reduction require in-game
validation. This is not a confirmed lighting fix.
