# R305 capture findings — 2026-11-03

Input: capture-manifest(20261103-161242).zip and
capture-part-001(20261103-161246).zip, process 52972.
The part SHA-256 matches the manifest; all 15 indexed file hashes and sizes pass.
The original files remain in the user's uploaded capture; no binaries are copied
into the repository. Reproduction uses tools/analyze-gi-writer.py on extracted logs.

All four whole writer exports pass validation: 34,942 binding observations,
including 5,559 alpha-tested observations. Each dispatch is 2560x1440. The tables
contain 7,620 / 10,126 / 5,705 / 11,491 rows of native CPU bindings, each preserving
two shader records per geometry. No zero roots, zero declared bounds, index offset
outside its declared bound, or material sentinel 65535 occurs in these snapshots.
This does not validate the GPU contents at those addresses.

The last bounded route report records 2,880 accepted GI dispatches, 2,879 scheduled
and finished writers, zero schedule rejections and zero frame/state/buffer/counter/
pipeline/layout/raygen/miss/hit/shape/exception failures. It is printed on entry,
before the current dispatch is scheduled, explaining the one-record difference.
The input counter includes unrelated native ray passes rejected by the GI scope;
it is not a GI failure count. All four exports report complete=1 and failed=0.

Within individual snapshots, 58 / 78 / 32 / 66 index/attribute-root-and-layout keys
have multiple material IDs. Material selection must therefore retain the exact
instance contribution and geometry-selected shader-table row; address lookup alone
cannot choose a material. Across snapshots, most shared single-material address
keys have changed IDs, but allocation identity was not captured, so address reuse
cannot be excluded. These comparisons do not prove material renumbering or stable
object identity. The new combined consumer fixture checks two bindings sharing
geometry but selecting distinct material/export rows, and rejects a stale material
mapping. Both index-width variants pass the local C++ test.

The existing instance probe reports four completed readbacks, zero failures, and
30,565 metadata matches with no missing/unavailable entries. Triangle observations:
38 readbacks, 224,346 triangles, zero out-of-range or nonfinite failures, and 53,904
degenerate triangles. Degenerates remain explicit and must not discard whole meshes.

The lifetime inventory still saturates: 302 AS owners occupy 536,790,528 bytes of
the 512 MiB cap, with 271 admission rejections. At the final report, 3,732 tracked
submissions are completed; 62,950 build/copy root resolutions were missing. This
is observer coverage, not proof of native resource corruption or the speckle cause.
Native retirement remains disabled. Full traversal ownership and safe integration
remain necessary before replacement GI dispatches can be enabled.

No repeat writer capture is requested. The matched GI setup, writer handoff and
whole-table CPU input format are now evidenced by this run. The capture neither
identifies a specific plant/hair material nor establishes the noise source, and
reconstructed CPU records must not be described as GPU shader-table readback.
