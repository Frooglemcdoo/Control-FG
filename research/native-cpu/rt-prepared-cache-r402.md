# R402 guarded prepared-record reuse

## Evidence driving this change
R401 supplied log probe-20261109-182815-195-64136.log: 15,468 samples,
1,346 cold, 13,969 unchanged, 153 invalidated, all 153 with identical payload;
zero untracked changes. This did not exercise a payload-changing invalidation,
so it cannot establish complete notification coverage. 5,632 preparation loops
handled 35,434,493 objects in 10,949,975 microseconds. Different routes/workloads
make comparison to older cumulative averages unsuitable for FPS claims.

## Experimental cache (not enabled in the game build)
Bounded shared 4,096-slot cache, known static providers with 0–4 geometries.
One nonblocking lock per preparation job; contention and recursion bypass cache.
Cache stores byte copies and pointer values only, never dereferences old pointers.
Each hit resolves current geometry and material tables, compares all raw geometry
bytes and material inputs (properties, mask pointer/value, cull mode, identity and
revision), and checks provider generation. Hashes select slots only; full keys
and inputs authorize reuse. Material variant, alpha and cull options are keyed.
Provider getters, live mask, current BLAS allocation/state/offset, transform and
prefix retain native behavior. LOD selection and dynamic callbacks are unchanged.
Hits replace ordinary assembly; they do not append duplicate scene records.
Misses run ordinary assembly and capture only after confirming its output matches
the current source. Growth, unknown/skinned providers and >4 geometries fall back.
R400 same-group batching remains available. R401 sampled observer is removed.

## Validation and limits
Extended native-machine-code assembly oracle: 16,384 exact replay comparisons
and 26,624 rejection checks. Cases include unnotified geometry/material mutation,
provider lifetime and material ID reuse, alternate material tables, options,
source changes, invalid binding, capacity flags/growth, nonempty outputs, prefix
wrap, NaN transform bits, changing mask and signed compacted BLAS addresses.
Rejected hits leave output and prefix untouched; corrupt completed output cannot
seed the cache. Existing 16,384 generic/provider/batch comparisons still pass.
Windows CI also checks production patch ABI/unwind and preparation/selection.
No claim of complete writer discovery or FPS gain: exact validation may consume
any preparation savings. Concurrent native mutation during assembly remains under
the native engine's lifetime/synchronization contract, as on ordinary assembly.

## Final game path
The exact-validation cache lost to ordinary preparation in the local warm-data
microbenchmark (~49–80 ns vs ~28–39 ns for 1–4 records). It is not enabled in the
DLL. This is synthetic CPU evidence, not the user's hardware or an FPS prediction.
The R401 observer and revision hooks are disabled too. The cache oracle remains
as research coverage, but its runtime wrapper is not included by the DLL.

The game path now reserves the complete object's already available vector space
once, writes live records directly and publishes each count once. No native
allocation callback can occur within this path. Known static/skinned providers
qualify; unknown or insufficient-capacity calls retain normal ordered appends.
Zero-geometry calls avoid forming pointers into absent geometry/material storage.
Direct rejection tests cover capacity exhaustion in each of the three vectors.
The Windows oracle prints a three-way synthetic timing comparison (baseline,
exact-validation cache, reserved writes). Timing is diagnostic, never a flaky
pass/fail assertion. Gameplay gain remains unproven.

The first Windows timings showed reserved writes slower for 1–3 records and
roughly equal at four; broadening the local benchmark showed benefits at 8–64.
The production route therefore uses reserved writes only for >=8 records.
The fairer broader benchmark dispatches both baseline and new routing through
volatile function pointers. It prints results for 1, 2, 4, 8, 16, 32 and 64 records.
Windows cache timings were closer than Linux (31–43 ns vs 32–46 ns), but those
warm-entry numbers exclude cache lookup, misses, locking and capture, so they
still do not justify enabling the persistent cache. No global speedup is claimed.

## Final evidence and handoff
Final code commit e4161a8b53e9815328cbc70c366a2c22ccd2f71f; local 23cd114.
The small-record dispatch regression was fixed by selecting reserved writes inside
AssembleProvider (AllowReserved template), retaining the common result/return path.
Windows standalone run 42678238628 passed all eight suites. Same-run baseline/new
nanoseconds: 1 record 14.43/14.79, 2 17.35/17.32, 4 24.35/23.83,
8 45.45/28.61, 16 61.01/41.89, 32 114.21/69.95, 64 221.17/124.71.
These are synthetic warm-data timings on CI, not gameplay or the user's machine.

Existing R393 instance upload capture has 48 samples. Consecutive contribution
prefix differences infer 467,590 per-object geometry counts (last object excluded
per sample); all differences even. Only 551 (0.1178%) have >=8 records, representing
0.7989% of inferred geometry. Therefore larger-record speedups have too little
coverage here to promise noticeable FPS improvement. Do NOT request another user
capture solely for this optimization. Exact numbers/source hash are in the JSON.

R402 is a bounded preparation cleanup and removes the R401 observer/hooks. It is
not ownership of AS management. Native continuation A8800–A8998 still performs
TLAS sizing, grow-if-needed buffers, map/copy/unmap of instance records. Native job
A89C0–A8BE5 still runs provider update/finalization callbacks, material shader-ID
patching at A8B60–A8B98 and AS submission at A8BCA. These are separate remaining
stages; do not claim their cost is removed by R402. Future work should target a
larger measured stage instead of another isolated append micro-optimization.
