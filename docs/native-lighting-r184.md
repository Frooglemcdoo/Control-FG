# R184: secondary-hit local-light view direction

R183 captures verified both isolation modes. The painting retained noisy
specular lighting with material emission removed, while its selected emission-
only region was zero. The elevator retained broad grain in shading mode and
sparse bright emission spikes. These findings prioritize reflected shading
without ruling out emission outliers. Camera placement between runs differs;
the comparison is by material/artifact, not exact pixel subtraction.

## Change

Native reflection resolve shades secondary point/spot-light highlights using
`V = normalize(-hitPositionInView)`, the direction back toward the camera.
At a reflected hit, the outgoing direction for the preceding path segment is
toward the primary reflecting surface. R184 changes that local-light direction
to `normalize(primaryPositionInView - hitPositionInView)`.

The provenance is explicit in the native shader: r2 is reconstructed through
g_mClipToView and retains reciprocal homogeneous w; r15 is the secondary hit
transformed through g_mWorldToView. The change uses r2.xyz*r2.w - r15.xyz.
It affects the local-light half vector, Fresnel and NdotV calculations that
consume r19.xyz/r12.w. It does not replace the independently calculated
environment BRDF view direction. A squared separation <=1e-12 falls back to
the original camera direction to avoid introducing a zero-length normalization.

This is a shading-correctness experiment, not proof that this mismatch causes
the captured variance. It may change highlight brightness and placement in
either direction. Native caps and all sample counts remain. Emission is restored
and still participates in the total-radiance cap; its isolated spikes are not
specifically fixed by this change.

There are four additional arithmetic instructions per shaded hit, zero added
temporaries, zero texture reads, no extra rays, no spatial blur and no additional
history. R172 load scheduling and R176 retry reuse remain. GPU performance and
image stability still require the user's hardware test.

## Test

Close Control. Extract the inner package beside Control_DX12.exe, keeping the
existing NVIDIA runtimes and ControlFGStreamline. **Launch normally**; the old
component-test launcher is unnecessary. Component isolation is compiled out.

Select NATIVE BLEND, RR on, FG off and SPEC4. Keep other settings fixed. F10's
bottom label reads **SPEC: HIT VIEW**. Inspect the painting and elevator after
standing still, then move normally. Report splatter, highlight brightness/shape,
and FPS; SAVE LOG. No exact camera matching or SPEC8/16 comparison is needed.
CAPTURE RR remains available but another capture upload is not required first.

## Validation

The bytecode transform rejects any source except the pinned native shader hash.
The audit checks coordinate provenance and scratch-register lifetime, reverses
every edit back to the accepted instruction ordering, and interprets the actual
emitted tokens against 10,000 independent direction/fallback cases. Portable
runtime tests verify exact shader routing, native fallback and COM ownership.

Windows CI validates the full shader's root signature/PSO and executes the
changed vector block on WARP against a shifted-camera reference. Fixtures cover
sample counts 0/1/2/4/8/12/16, capped/uncapped output, partial dispatches, sentinel
termination and coincident-position fallback. They do not execute complete
scene lighting or establish hardware FPS/image quality.

`SPEC_HIT_VIEW applied=1 native_fallback=0` confirms PSO creation used the new
shader. Unsupported shaders and failed creation retain the native path. This
branch is experimental; no production branch or release is changed.
