# R276 follow-up: guides and native hit contract

The user reports better overall image quality with reflection clamp bypass,
no old boiling, and unchanged plant/hair speckles. Use bypass as the comparison
baseline. This does not establish a universal quality or performance result.
No repeat of that A/B is needed.

## Recovered guide checks

`tools/audit_rr_capture_guides.py` examines all six recovered R271 frames:
4578, 4579, 5286, 5287, 6069 and 6070. The checked-in JSON includes buffer hashes.
These are older recovered inputs, not R276 output captures.

- All checked guide and GI components are finite.
- Maximum normal length error is below 0.00084; roughness stays in [0,1].
- Roughness differs from decoded native smoothness by at most 0.000489.
- Full-coverage diffuse replay matches exactly. Partial hair intentionally blends
  and is excluded from that equality check.
- Responsivity is nonzero only inside tagged valid hair coverage.
- GI RGB is exactly preserved across bypass. Alpha is intentionally repacked
  to the red component; full RGBA equality would be an invalid requirement.

These checks rule out basic corruption in these captures. They do not establish
correct material semantics, temporal alignment, specular response, or the cause
of visible speckles. Five synthetic tests cover truncation, NaN detection,
full-coverage mismatch, responsivity leakage and intentional partial-hair/alpha
exceptions.

## Native hit-data findings

The pinned ray-library variant 0 writes hit position as view-space XYZ and UV.y
in W. Before WorldToView, its world position is origin plus direction times
(RayTCurrent - 0.01). GI6 transforms that XYZ back using ViewToWorld at CB0
registers 6..9. The bias is in scene units; it is not a proven defect.

The miss shader writes material sentinel 65535 and a position, but does not
write a surface normal. A reuse implementation must handle misses separately;
finite contents of a normal allocation do not establish a valid surface hit.
It must also undo the stored coordinate transform before computing connection
geometry. These findings come from static producer/consumer disassembly, not
proof that runtime matrices match. The sampling audit now checks the pinned
producer instructions and consumer transform against the actual shader bytes.

## Next implementation boundary

No new clamp, light dimming, or material exclusion is justified by these results.
Before enabling indirect sample reuse, recover the primary/secondary coordinate
and density contract and provide fresh connection visibility. The existing hit
textures alone do not supply a complete, validated reuse estimator.

No game shader or executable is changed in this follow-up. No new build or
speckle fix is claimed. R276 notes were corrected: effective reflection-clamp
changes already reset RR history through rr_evaluation_entry.h.
