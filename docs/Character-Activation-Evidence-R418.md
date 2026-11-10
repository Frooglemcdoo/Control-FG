# R418: repair the missing activation evidence

## Supplied run

`probe-20261110-220204-549-69944.log` contains 13 RT lifecycle lines.
SHA256: `5dad3ad8ac350cf2449da126934a7f25f7085204277e2b9b1ead36c6ccc5c95f`.
It reports `diagnostics=0`, owned preparation, native selection and disabled
expensive cache/heap/allocator diagnostics. It contains no character activation
events. The retained R411 subsystem marker names do not identify the package.

## Confirmed source defect

Before R418, `Log` returned immediately when on-demand diagnostics were inactive,
except for its RT-prefixed lifecycle allowlist. Both
`RR_CHARACTER_BASE_ALBEDO_R417` and `RR_HAIR_OPACITY_READY` were rejected before
formatting. Therefore asking this normal-run log to establish activation was
incorrect. Their absence proves neither success nor failure of the replacements.

R418 adds an exact marker-boundary allowlist for these two existing creation
events and one startup event. Unknown names, partial prefixes and per-frame
character/guide messages remain blocked. The existing secondary release-log
policy admits all three markers. Full performance-build logging remains disabled.

Each existing replacement cache has at most 32 records and returns on cache hits
before reaching its creation log. No new per-frame counters, GPU work, readbacks,
shader changes, ray changes or filter settings were introduced. The startup
message identifies R418 and states that rendering remains R417.

## Interpretation of the next normal-run log

- Startup marker establishes that this support build opened the log.
- A skin/hair activation event establishes validated replacement record creation.
- Creation does not establish that those pixels reached the final RR image.
- Missing activation with the R418 marker requires investigation of admission and
  replay coverage; it is not evidence that the original noise is solved.

The user reports continued jaw/neck noise and red hair speckles after R417.
Projector, desk and plant acceptance belongs to the prior run and must be retained.
R418 is not a claimed visual fix. No repeated capture sequence is requested; the
next log can come from ordinary play in the affected scene.

## Local validation

- Compiled C++ allowlist test: valid events admitted; prefix collisions, null,
  empty and continuous guide events rejected; secondary release policy checked.
- Existing four-family captured skin reflectance audit and shader hash test pass.
- All 132 protected rendering baseline files pass.
- Source inspection confirms production logger integration and capped cache paths.

## Build validation

Runtime commit: `77cdc4c89497a8fe583ec22985ec2d198c7c4e49`.
Windows workflow 42915674454, job 130177331111: all 60 steps successful.
The C++ policy test compiled with warnings as errors and ran successfully.
The four captured native skin audits, existing shader match test and protected
baseline check passed. The downloaded archive passed CRC and published digest
verification; the x64 DLL contains R418 labels and all three lifecycle markers.

Package: `ControlTOH-R418.zip`, 6,797,100 bytes, 83 entries.
ZIP SHA256: `f7ddce4f9445416b0ee92f5d2c48eed31f60306c546747081d6171240e64652e`.
DLL SHA256: `ddf935753d798b02c159ad047606810f6724df167d529dfafc48f9341bbad305`.
Live-game activation remains unverified until the next normal-run log.
