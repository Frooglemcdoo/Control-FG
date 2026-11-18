# R466 integrated candidate

The checkpoints below document development history. R466 now integrates regional lookup in tracing and evaluation, GPU training packing, five-stage publication, larger immutable buffers and both 4/10-ray launchers. Receiver keys use two-metre cells and normal octants; source bounds use a conservative fixed 0.433 metre radius. Missing or expired regions fall back to cosine sampling. Native geometry, hit records and material evaluation remain in use. This is a first live integration candidate, not a completed replacement of the native GI stack.

Automated validation covers packing, colliding regions, invalid inputs, empty publication, host recording and shader compilation. Live direction-mismatch flags, source discovery, temporal stability, illumination bias and total frame cost remain to be evaluated in game. No noise elimination or performance recovery is claimed.

# GI replacement overhaul: first implementation checkpoint

Production baseline remains R465. These components are intentionally not hooked
into the game: they establish receiver ownership before changing sampling.

Implemented:
- Shared C++/HLSL receiver-key and guide validity contract, with four-probe bounded
  lookup. Different regions do not consume a shared pair of globally ranked guides.
- Version, age, future-frame, normal-group and invalid-bounds rejection. Missing
  proposals fall back before ray sampling; this component never reuses radiance.
- CPU collision/expiry/invalid/budget tests and a real D3D12 WARP fixture for
  receiver identity and rejection. This is lookup validation, not lighting validation.
- Reproducible static audit of both pinned native GI libraries after applying the
  production shared-hit patch. Native material, normal, UV and endpoint stores stay
  intact; owned rays publish RayTCurrent without the native 1cm distance offset.
  Endpoints are transformed by world-to-view rows 2–5; the evaluator uses inverse
  rows 6–9. No format change is inferred from the shader's float resource type.

Open before integration:
- Explain live direction-mismatch flags using actual producer/evaluator constants
  and storage precision. Static agreement is not proof of live agreement.
- Build and publish the receiver-local proposals on GPU with bounded memory and
  training cost, and freeze the same distribution for tracing and evaluation.
- Validate source discovery, sparse regions, candidate churn and moving geometry.
  The existing quarter-metre cell and normal-octant key is a prototype, not a
  commitment to the final spatial resolution or a surface identity guarantee.
- Compare mean illumination, variance and temporal behavior with an independent
  reference across scene types before claiming the noise problem is fixed.
- Measure complete frame cost. Four probes bounds lookup work, but does not prove
  it is fast enough on the user's GPU; cache build and memory traffic also count.

Validation commands:

    g++ -std=c++17 -O2 -Wall -Wextra -Werror tools/test-gi-regional-target-lookup.cpp -o regional-cpu
    ./regional-cpu
    dxc -T cs_6_0 -E main -WX tools/gi-regional-target-validation.hlsl -Fo gi-regional-target-validation.dxil
    python tools/audit-gi-hit-writer.py --dxc PATH_TO_DXC --output hit-writer-audit.json

Windows GPU checks run through Validate GI overhaul foundation. Local shader audit
used official DXC v1.9.2609; archive SHA256
96faadc7f5c282d2ffda49804beb4c3ee38127bc252b723234e3c5cdf7aa39a1.
Future game builds remain one package for both ray counts. No new game test is
requested at this checkpoint; the replacement runtime is not complete.

## GPU construction checkpoint

Added five bounded compute stages: clear, claim receiver region, rank primary,
rank distinct secondary, publish immutable table. Each training record performs
at most four region probes. Claim publishes an atomic index into immutable input,
so another thread never reads a partially initialized multiword key or spins on
an owner. Hash collisions can exhaust the probe budget and cause fallback; they
must never mix training across receiver regions.

The new GPU fixture builds two colliding receiver regions with separate bright
sources and a covered duplicate, rejects invalid-version/NaN records, consumes
publication through regional lookup, and verifies that an empty next frame clears
old publication. The separate GPU lookup fixture verifies consumer-side identity.

This prototype takes source bounds as input. Producing trustworthy bounds from
live ray data, training coverage, temporal publication and game integration are
still outstanding. Five dispatches versus R465's four is a cost to measure; no
speedup follows merely from bounded probes. With 4096 slots, cell scratch is
64 KiB and one published table is 336 KiB, before training, retained versions and
any allocator overhead. No extra ray traces are introduced by construction.
