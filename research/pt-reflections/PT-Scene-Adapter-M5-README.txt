PT Scene Adapter M5 - component build checkpoint

This build integrates native-layout shader table conversion into the private DXR
host. It does NOT integrate the renderer into Control and is not a drop-in mod.
No additional game capture is requested. Earlier M4 hardware tests remain valid
for the unchanged geometry shader. CI executes the new conversion and all 24
geometry path cases, in addition to M2/M3 regression tests.

The existing overview export contains 27,750 explicit native local-root records,
2,418 unique export/binding/constant combinations and 508 material IDs. All indexed
records use 16-bit indices and attribute strides of 16 or 20 bytes, with normal
at byte 4 and UV at byte 12. These are captured layout facts, not live identities.
The included source hashes make the audit reproducible with
research/pt-reflections/tools/map_entrance_shader_tables.py.

Conversion preserves record indices and replaces the 96-byte native layout with
128-byte private records. It requires known native export identifiers and a
verified position-buffer mapping. Mismatched material IDs, bounds, allocation
epochs, or unknown layouts return failure with no partially published record.
The GPU fixture uses synthetic native identifiers and owned buffers; it does not
assert native resource lifetime or position correspondence.

Remaining: obtain verified native position-buffer associations for static BLAS,
connect resource lifetime tracking, native material/light evaluation and output
composition. The exported local roots provide index and shading-attribute
buffers but NOT the position stream. Do not infer it from adjacent resource IDs.
Unsupported records currently fail the conversion; a caller must keep native
rendering for the whole affected dispatch. No partial private table may be used.

NVIDIA self-intersection library: see NVIDIA-SIA-LICENSE.txt.
The optional Run-PT-Geometry-Test.cmd repeats the geometry suite and writes
Geometry-Test-Results.txt; a repeat of that hardware test is not required for
this CPU conversion-only update.
