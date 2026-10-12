# Opaque R2: preserve native terms through intensity clamping

The supplied R1 log matches the packaged raygen SHA f814f566389284cedf478a02d84160f02d3ab4fcedcbcea9ee9aaaa629b8436f.
It records PT_REFLECTION_AB_APPLIED toggling added_composite while
ray_tracing_unchanged=1. The source agrees: the toggle bypasses only the added
composite. Consequently the one-primary-sample layout does not explain this
particular on/off difference. The earlier attribution to that layout was wrong.
Private secondary storage and two primary samples remain a separate improvement.

The actual replacement was Output(retained+bounce)-Output(nativeFull), with
Output applying a luminance clamp to the entire RGB sum. The claim that retained
diffuse/direct/emission cancels was false when either sum is clamped. A hot red
secondary sample can scale down the retained green/blue channels by over 90%.
Existing CPU composition tests did not include that nonlinearity.

R2 computes the original native scale from nativeFull, uses it unchanged on
retained lighting, then limits only the replacement specular to the remaining
intensity budget. Depth fade is applied once to both terms. The original native
reference/parity admission and final substitution delta remain in place.
The native reference scale also applies to the proposed bounce, so substituting
the original indirect specular reproduces the original result. Removing it does
not amplify retained lighting, including when the original sum was clamped.

The shared HLSL scalar functions execute directly in the C++ regression: 300
colored cases, zero/disabled/positive clamps, dark and hot bounce, equal replacement
and retained-channel preservation. The old hot-bounce failure is explicitly
reproduced. Shader compilation and existing gates cover integration constraints.

This is a confirmed mathematical defect and a targeted candidate for the supplied
detail/color loss. No per-pixel GPU capture establishes that this clamp caused
every difference in those screenshots; in-game comparison is still required.
The game’s later filtering/clamping remains unchanged. All raygen and glass
binaries should remain byte-identical to R1; only normal/verify opaque composite
binaries change.
