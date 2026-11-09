# R402 guarded prepared-record reuse

## Evidence driving this change
R401 supplied log probe-20261109-182815-195-64136.log: 15,468 samples,
1,346 cold, 13,969 unchanged, 153 invalidated, all 153 with identical payload;
zero untracked changes. This did not exercise a payload-changing invalidation,
so it cannot establish complete notification coverage. 5,632 preparation loops
handled 35,434,493 objects in 10,949,975 microseconds. Different routes/workloads
make comparison to older cumulative averages unsuitable for FPS claims.

## Implementation
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
