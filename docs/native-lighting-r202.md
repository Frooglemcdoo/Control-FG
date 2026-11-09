# R202: compare native PCF center with centered RT

R201 verified that most of the sampled missing-light disagreement is already
present in raw spotlight visibility: 266 of the affected light's 289 darker
samples were blocked before filtering. The native map permits partial light at
many of those locations. The accepted RT baseline targets the emitter center;
the native PCF branch averages 16 offset depth comparisons.

## Capture

Keep RR ON, SPEC Native, hit distance OFF and the accepted RT settings including
Ray Traced Contact Shadows. Select Native Blend, then MAP GUARD OFF. At the
outside-window view with missing light, F10 -> TRACE SHADOW and hold still.
Send the Shadow-R202 folder's log, manifest and three PNGs. One trace is enough;
no inside-camera trip, settings sweep or raw-display comparison is needed.
MAP GUARD ON is still a valid capture if already recorded; its state is logged.

## Measurement

The capture-only native selector now additionally samples its existing PCF atlas
at the unoffset native center. It uses the same atlas, sampler, LOD, receiver
comparison depth and comparison direction as the existing PCF taps. This is one
center sample, retaining the native sampler's texture filtering, not a claim of
an unfiltered texel lookup or the same ray origin/projection/bias as RT.

The sample follows the exact winning-light predicate into the previously unused
fourth map-sidecar channel. Values are 0 (blocked), 1 (lit), or -1 (no supported
PCF branch for this candidate). The explicit map_center=1 header distinguishes
this measurement from older captures whose fourth channel was zero. Existing
map visibility/complement/ID, raw and filtered RT, light records and ray hits are
retained. The observer does not change any normal lighting output.

For PCF-only lights with fade 1 (including the affected captured spotlight):

- Map center blocked and raw RT blocked, but 16-tap map partially lit: the map's
  offset sampling admits light that the center tests do not. This establishes
  a footprint difference at that sample; it does not identify a correct fix.
- Map center lit and raw RT blocked: averaging alone does not explain it.
  Projection, receiver bias and native-versus-RT caster representation remain.

Combined static/VSM layers and authored fades require separate interpretation;
the analyzer restricts center categories to the PCF-only layer configuration.
Do not infer missing final radiance from map versus unfaded RT alone.

## Scope and verification

R200/R201 gameplay rendering is retained. Only the capture-only selector gains
one private register and one additional atlas sample per evaluated PCF candidate.
No rays, buffers, allocations, or readback stages are added. This extra work runs
only on a requested trace. Every original selector instruction and normal UAV
store is verified unchanged after stripping private instrumentation.

A WARP fixture executes the actual emitted center sample, depth comparison,
unsupported sentinel and winner selection on a patterned atlas. Full shader PSO
validation and existing capture/decoder checks also run. The decoder rejects
invalid center values and unknown schema versions; older logs remain supported.
Actual in-game center data remains to be verified from the next trace.

This is a new measurement, not a replay of R162 projection rays or R160 map
priority. The rejected wide emitter sampling is not restored. Close-shadow and
reflection fixes remain intact. Experimental branch only.
