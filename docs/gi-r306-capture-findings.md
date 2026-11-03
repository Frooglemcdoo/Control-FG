# R306 capture findings

The supplied 20261103-164114 part ZIP matches the manifest SHA-256
`4fcfd268e84341518dff5f97f7295db732de90062f85552bf19abe68195c7368`.
All 15 indexed files match their sizes and hashes. Process ID: 47912.

The lifetime hook installed (`ready=1`). Its final bounded report observed
12,800 wrapper releases: retired=0, untracked=12800, stale=0, invalid=0.
Therefore R306 did not demonstrate AS retirement or fix inventory saturation.
The final inventory contains 299 owners / 536,320,256 bytes and 276 rejected
admissions. Root resolution reported 93,845 resolved and 43,179 missing.
All 3,472 reported tracked submissions completed. The older `native_retirement=0`
field in GI_AS_LEASE_R304 is hardcoded and must not be used to infer whether the
new hook installed; the new installation/retirement records are authoritative.

Four writer tables validate without errors: 7,620, 9,041, 5,690 and 13,293 rows,
35,644 total, including 5,105 alpha-tested rows. All exports completed with zero
export failures. The final bounded GI route report has 2,880 accepted dispatches,
2,879 scheduled/finished workers, zero schedule rejections and zero reported
frame/state/layout/shape exceptions. Its report-before-schedule ordering explains
the one-dispatch difference; these counters are not GPU-completion evidence.

## Interpretation and next implementation scope

Do not request a repeat capture. R305 and R306 provide valid full CPU binding
snapshots and the writer handoff. Neither proves a visual speckle fix.

The R306 wrapper hook covers 0x25f00 only. Offline disassembly also contains
resource-pointer deferred enqueue calls outside that wrapper, including 0x36241,
0x36338 and 0x36cd5. These require tracing their callers/owners before selecting
an AS retirement bridge. Zero matches alone does not establish whether retained
AS objects are live pool allocations or released through another path. Do not
raise the inventory cap, call untracked wrappers corrupt, or make a new capture
build solely to distinguish these alternatives while static evidence remains.

The user has authorized an experimental integrated GI renderer with native
fallback and iteration on actual rendering. Keep diagnostic inventory cleanup
separate from rendering admission: the inventory limit is not itself a GI
speckle cause. Prefer an in-place native-pass integration where native bindings
and lifetime can be preserved; any additional dispatch or resource retention
must explicitly own the resources it uses. Existing source PDFs, geometric
normals, alpha visibility and destination-dependent secondary shading contracts
still apply to spatial/temporal reuse. A shader substitution or clamp is not a
completed ReSTIR GI implementation and must not be presented as one.
