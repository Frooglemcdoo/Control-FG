M12R74 - material history independent of table slot numbers

R73 identifies the dominant rejection gate. At frame 2413, 1911 of 1917
covered samples have matching normals, but only 300 have matching material
indices. At frame 2893 the corresponding counts are 1979 and 449. R73's four-tap
lookup helps some pixels but does not resolve this broad material disagreement.
These counters do not by themselves prove why indices differ.

R74 stops requiring equality of current and previous material-table slot numbers.
It requires matching 128-bit fingerprints: canonical geometry-binding membership
(64 bits) and all 84 Part3 words plus both Part1 words (64 bits). A small GPU
prepass snapshots these fingerprints once per material, rather than rehashing
records per pixel or retaining references to mutable native material contents.
The native buffers/scene are consumed at the existing admitted producer point.

Missing, out-of-range or simultaneously ambiguous geometry keys reject history.
Changed geometry membership/addresses, flags or parameter contents also reject
history. Existing coverage, finite-radiance, depth, normal and sample-count tests
remain. Same-frame spatial filtering still requires the actual material index.
History buffers keep their 32-byte stride; the extra per-frame material snapshot
is about 109 KiB, with a roughly 55 KiB CPU-key upload. No additional rays.

Geometry fingerprints are conservative correspondence aids, not permanent object
identities. They do not version texture descriptor/content changes or supply
object motion vectors. Geometry-set changes can still cause unnecessary resets.
Missing lights, area/emissive sampling, zero environment intensity and native-pass
removal are separate unfinished work. In-game quality/performance are unverified.

GPU regression coverage includes a changed slot with identical material data
(retains history), changed data at the same slot (rejects), geometry changes and
flag changes (reject), plus existing camera/normal/depth/admission cases. CPU
coverage checks row reorder, duplicate rays, renumbering and ambiguous groups.

New counters: material_match remains raw slot equality; stable_material_match
reports fingerprint agreement; reindexed_accepted counts pixels whose accepted
history includes a different slot. These distinguish actual recovery from a
hypothesis about renumbering.

Close Control and replace all package files. Keep current PT/game settings.
Do not rerun PT-Enable when already enabled. Run Collect-PT-Primary-M12R74.cmd
for the next result bundle.
