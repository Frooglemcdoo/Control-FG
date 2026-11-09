# R401 invalidation observer: reuse remains disabled

## R400 evidence

Full uploaded log probe-20261109-172654-588-64048.log (91,448 bytes): all hooks
installed, no error-like lines, 163,048 reused geometry preparations / 70,809,926
geometry records (0.2303%). Loop: 9,728 samples, 53,165,538 objects, 53,129,506
instances, 15,701,120 us (1.614013 ms). R400 processed 26.416% fewer objects than R399.
No controlled FPS gain established. Batch-only tuning is not the next target.

## Recovered events

Exact renderer hash remains ebfb5b47cebc2d4482e912b8090bd343f717bab8a178b5a6385dec9105e7c433.
Static provider constructor 1504C0 has six Win64 arguments; original stack args
at +28/+30 are alternate and manager. Destructor 150AA0 retires allocations and
frees its geometry vectors. Constructor builds the +20 geometry table. Ready
150C70 only latches +A8; this is not an object/material generation. State machine
150CC0 moves through BLAS build/compaction states 0..4; address getter 151080
switches allocation at signed state >=3. Future reuse must refresh BLAS addresses.

Exports identify Material::setProperties (9DE90) and setCullMode (9DEB0). Both
write a field then tail-call 1B6910. 1C9CF0/1C9CFE also alter properties and tail-call
that notifier. The notifier coalesces queue insertion via byte +99C; observing
that boolean after it is cleared cannot recover every intervening mutation.
Material constructor calls notification at 1B66E3 after assigning native uint16
ID +99E; copy constructor at 1C9757 does likewise; loader calls it at 1B70BA.
Capture every notification at entry, even if native dirty is already set.

Direct-offset write scans are not proof of all mutation routes: deserialization
passes an address (e.g. 1B7015) to another routine. Shader property masks are
indirect +1F8 reads; graphics settings write a default mask at 137A85/137A91.
Future eligibility must include that mask value and current material mapping.
No full writer-completeness claim is made by R401.

## Observer and entry patches

Three hash/byte-verified entry detours count provider construction/destruction
and material notifications. Constructors/destructors increment one of 4096
provider revision buckets (collisions invalidate conservatively). Notifications
increment one of 65536 material revision slots keyed by current native ID.
No pointer is retained/dereferenced by these counters. Original functions always
run, with constructor arguments and return preserved.

Entry bytes: constructor 44894c2420 (5); destructor 40574883ec40 (6); material
405556574883ec30 (8). Near E9 patches jump through absolute entry relays. Copied
prologues have registered unwind records. Continuation uses MOV R11/JMP R11,
not an epilogue-shaped RIP-indirect jump following an allocated stack. R11 is
volatile and these three original functions initialize it before any use.
Code pages and unwind tables live for process lifetime, including rollback.

At most 256 normal assemblies are observed per sampled loop (one every 64).
Only known static providers with <=4 geometry records are admitted. A per-worker
4096-entry numeric table compares provider/owner/variant/options keys, a guard
hash (provider lifetime revision, source pointer/count, current material pointers,
material IDs/revisions, property mask pointer/value) and payload hash (prepared
geometry and aggregated instance flags). Transform, contribution base and BLAS
address are excluded and must always be refreshed by any future optimization.
Batch-replayed records are not sampled. Counters flush per sampled loop; logs
report every 512 loops with other preparation summaries.

An unchanged guard with changed payload is an untracked change and rejects the
proposed invalidation model. Hash collisions, sampling limits, key eviction and
worker migration mean zero observed mismatches is not proof of correctness.
This table never supplies rendering data or authorizes cache reuse. Original
records, native updates, sizing/upload and AS builds all remain active.

## Required gate

The new native hook routing needs in-game validation before any persistent cache
suppresses work. This run is explicitly diagnostic, not another claimed speedup.
It also measures whether revision churn would erase cross-frame reuse. Existing
logs lack these events, so repeating their FPS-only comparisons cannot supply
that evidence. Keep source/lifetime audits and exact validation requirements for
any eventual cache; do not promote sampled hashes into a correctness guard.

Reproduce exact patch bytes and notification edges with tools/rt-scene-probe/audit-revisions.py and the verified renderer. The script deliberately reports complete_writer_coverage=false.

Sampling rotates through 32 windows of 256 ordinary assemblies instead of always
inspecting the first group. The first eight untracked changes log a changed-word
mask and first old/new word. invalidated_same_payload measures conservative revision
changes that did not alter the observed preparation output.
