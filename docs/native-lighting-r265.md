# R265: verified foliage albedo coverage

Install the complete package beside Control_DX12.exe. Use RR ON, Native Blend,
GI RAW BYPASS, GI samples 6 and FG OFF, matching the successful R264 floor test.
Inspect the affected plant under the same lights, stationary and moving; recheck
the floor. Send compact logs and the visual result. A new large plant capture is
not required initially. The admission record is RR_FOLIAGE_R265.

This build admits only the exact R258-captured foliage shader combination into
the textured albedo replay. Both native and selected vertex bytecode must match
each other AND the captured 7692-byte vertex shader. The original 17648-byte
pixel shader and selected 1072-byte albedo shader must also match the evidence
exactly. The ordinary vertex/parser/binding/target contract checks still run.
Unknown combinations fall back to existing material guides. Material IDs and
object names are not hard-coded.

The verified replay samples the native base-color texture using the same UV,
material texture index and sampler, writing RGB and alpha 1. It is not tagged as
hair. No replacement GPU shader is synthesized. The existing primary-view draw,
borrowed depth target, draw-range validation and restoration are retained.
This corrects a known input coverage gap; it does not prove all foliage variants
or all speckles are fixed. Accepted batch logs do not prove per-pixel coverage.

Foliage validation has its own bounded Session so it does not consume the
character/hair validation budget. Exact evidence is part of the per-session
cache key, preventing an ordinary structural validation result from bypassing
the stronger semantic match. Native original PS is copied/checked only once
per distinct pair/evidence combination per preparation. Extra foliage replay
work may have a cost; in-game performance remains to be checked.

R264's 256-phase GI sampling is retained, including its diagnostic scope across
native/RR-off modes. GI filtering, ray count, reflection settings and DirectStorage
are unchanged. Hair guides and responsivity are unchanged: the video shows an
issue dependent on lighting, but does not distinguish diffuse input noise from
specular-highlight reconstruction. A later buffer capture with Jesse's hair in
the problematic light can use the existing CAPTURE RR / PACKAGE RR controls.

Validation includes the actual recorded vertex/pixel parser contracts, byte
mutation rejection, Windows resident-shader reads, exact original PS rejection,
vertex mismatch rejection, and cache-policy separation, plus existing tests.
