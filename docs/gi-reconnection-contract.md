# GI reconnection prototype after R278

R278 verified same-pixel accounting. It did not verify transporting a sample to
another receiver. This branch adds original shared C++/HLSL connection geometry
and a DXR visibility ray-generation prototype. Neither is bound into the game.

## Probability conversion

For source receiver s, target receiver t and true secondary surface y, let ds/dt
be connection lengths and cs/ct be the cosines at y using its geometric normal.
For a fixed differential surface patch, dOmega = cosine * dArea / distance².
Therefore the source-to-target solid-angle Jacobian is
`J = (ct/cs) * (ds²/dt²)` and the mapped proposal density is `p_target = p_source/J`.
The shared implementation tests identity, doubled distance, an angled connection,
and the reciprocal transform. It rejects invalid/nonfinite inputs, missing source
PDF/geometric-normal verification, stale frame tags, zero-length connections,
miss/invalid materials, unsupported backfaces and connections beyond the source
or target finite ray range. Two-sided treatment must be explicitly supplied from
the material policy. It does not clamp grazing weights or cap bright energy.

This is ONLY a proposal-density transformation. It is not a receiving-surface
BRDF, a target importance density, a reservoir weight, MIS normalization, or a
proof of unbiased resampling with the near-field fallback estimator. Those must
still be implemented and tested together. A rejected reuse candidate must leave
the verified local estimator available; it must not zero the pixel's GI.

NVIDIA's [ReSTIR GI integration guidance](https://github.com/NVIDIA-RTX/RTXDI/blob/a6efab966b7c3b272da0461578eb56ac61c7cbff/Doc/RestirGI.md)
requires the source solid-angle PDF and secondary surface data. Its
[shader API](https://github.com/NVIDIA-RTX/RTXDI/blob/a6efab966b7c3b272da0461578eb56ac61c7cbff/Doc/ShaderAPI-RestirGI.md)
distinguishes reservoir combination and final normalization, including the
selected sample's target density. Our conversion does not replace those steps.

## Visibility and foliage

`gi_connection_visibility.hlsl` traces one finite connection, returning frame and
connection identity plus separate valid/visible flags. It intentionally uses
`TraceRay` with first-accepted-hit termination and no forced-opaque flag. The
existing `rt_lights_visibility.hlsl` provides the repository precedent for linking
Control's native contact-shadow hit/miss handlers and retaining their material
alpha tests. An inline opaque-only query is unsuitable for general foliage reuse.

The new shader requires explicit verified native bindings, shader-table layout,
instance mask, endpoint biases and a current-frame candidate. It does not supply
replacement native hit/miss shaders, assume an arbitrary table layout, or reuse
screen-space neighbor visibility. Compilation alone does not validate the native
payload convention, mask, material alpha path, scene lifetime or final binding.

## Producer evidence still needed

The pinned producer audit records a position at `RayTCurrent - 0.01`, transformed
to view space. R278 records that consumer position after its world transform.
Do not silently treat this biased point as the true secondary surface point for
the area/solid-angle conversion. Similarly, the captured normal has not been
established as the required geometric normal, and the cosine PDF statement is
conditional on uniform source coordinates and a valid orthonormal basis.

The `verified` flags must stay unset until a producer adapter establishes those
facts. Destination-dependent secondary shading identified in GI6 also still needs
reevaluation; the old source-weighted RGB cannot be reused as destination radiance.
The current R278 trace cannot by itself satisfy this contract. No new capture is
requested in this checkpoint; the next investigation is the producer data flow.

## Validation performed

`g++ -std=c++17 -Wall -Wextra -Werror tools/test-gi-connection.cpp` passes the
shared calculation's analytic and rejection cases. Official DXC compiles
`src/shaders/gi_connection_visibility.hlsl` as `lib_6_3` with `-WX`. No DXR scene
execution, native game linkage, quality improvement or performance claim is made.
Native Blend, DirectStorage and the shipping GI calculation are untouched.
