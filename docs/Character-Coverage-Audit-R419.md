# Character destination-guide and coverage audit

2026-11-11. Research only; runtime remains R419.

## Finding that changes the next step

The native live-noise character shaders choose a coverage sample before decoding the reconstruction map. The R360 filter assumes sample index zero whenever sample count is <=1. Those are not equivalent conditions.

Pinned skin-live-noise-b (SHA 004d3c430bdb795fa3201d0cf737d9ec1206443377a1d746eb47b6ba613e2b4f):
- dwords 428-446 enable a single-sample noise path using common_depth_general[2].z and [2].y.
- dword 278 samples material texture slot 26; opacity participates in coverage selection.
- dwords 578 onward include the native frame value common_system[69].z in a pixel/noise hash.
- dword 1227 intersects effective coverage; 1234 applies firstbit_lo.
- dwords 1292-1306 multiply this index by two, add pixel parity and shift the packed map.

Pinned hair-live-noise has the analogous sequence at 1222/1229/1287-1301. Neither captured pixel shader has a discard instruction in this path. That alone does not prove visibility: raster/depth state and previous passes still determine which invocations contribute.

For zero effective coverage, firstbit_lo returns 0xffffffff. With the native 32-bit shift convention, the selected map bit offsets become 30/6/14/22 instead of sample-zero offsets 0/8/16/24. No claim is made that those bits are necessarily uninitialized or invalid in the actual map. The proven fact is that the native selection and the filter's neighbor assumption can differ.

## Verification

Expanded tools/test-character-native-remap.py to execute sample-zero and zero-coverage sentinel cases across all twelve pinned families, four pixel parities and 64 map words: 6,144 passing cases. Four deterministic packed words demonstrate differing selected map bits in every parity. Existing coordinate decoding is still correct for its stated sample-zero domain; the new finding concerns eligibility, not parity arithmetic.

This test begins after firstbit_lo and supplies its result. It does not simulate native opacity/noise textures, prove zero effective coverage occurs on visible artifact pixels, or establish temporal frequency in the game.

## Destination depth and hair limitations

The native skin/basic and hair/basic shaders compare g_tDepthCopy sample depth against interpolated clip z/w with tolerance 2.47900005e-7, but only in the sample-count >1 branch. Our reconstruction filter deliberately runs only in the complementary <=1 path. Therefore that native check cannot be cited as proof of destination identity for our filtered pixels.

Hair-depth also writes SV_Depth using texture slot 22 and a material parameter (Part3 offset 92). An unadjusted clip-depth identity check cannot be applied to this family without accounting for that offset. Hair basic/live-noise have their own coverage behavior; none should be treated as opaque skin merely because a nearby GBuffer sample exists.

## Decision

Do not enable destination-anchored reconstruction yet. First preserve and validate the native selected coverage index before reconstructing neighbors. A prospective guard should admit only supported sample count and valid sample-zero selection; invalid/unsupported cases must preserve the native read. Snapshot the index immediately after firstbit_lo because native remap code subsequently overwrites its register.

This is a candidate correction to our filter's eligibility, not yet a fix for native zero-coverage shading or the user's noise. Clamping the native sentinel to zero would alter native lighting and is not justified. Adding opacity discard to the RR guide replay would likewise be premature without matching depth/coverage semantics.

Existing R378 capture inventory has depth, motion and GBuffer exports but no named reconstruction-map/coverage export or native character constant-buffer dump. Its stride-four exports also cannot resolve 2x2 correspondence. No stationary test or repeat generic activation capture is requested. Next work can implement and validate a fail-closed coverage guard offline; a visual-fix claim requires evidence beyond these synthetic cases.

## Offline candidate completed

Added tools/character_coverage_guard.py, deliberately not imported by the production generator. It snapshots the native selected index immediately after firstbit_lo and wraps the existing reconstruction in an index==0 condition. Unsupported selections preserve the native diffuse result and issue no extra gather reads. The existing sample-count, finite-value and boundary gates remain inside.

Validation: tools/test-character-coverage-guard.py passed 36 emitted guard cases across twelve exact reversible bytecode transforms. Bindings and constant-buffer descriptions remain unchanged. This adds one temporary register to a prospective shader. Production runtime and package remain unchanged. GPU PSO validation and real visibility/coverage behavior are not established by these CPU tests.
