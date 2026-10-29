# R206: Native Blend GI-filter ghosting isolation

R205 is the accepted lighting/reflection baseline. The supplied carpet-motion
video shows trailing around the moving character; the user reports Native
Blend is affected while Native is clean. This does not establish a cause.

Only the Native Blend family now returns false from PreservesGiFiltering.
Under full RR, this selects the existing GI bypass used by Native rather than
preserving the game's temporal/spatial GI filter before RR. Indirect lighting
itself stays enabled. The existing frame latch, signal-change RR reset, native
history release and bypass validation are reused. RR-off rendering is unchanged.

All four Native Blend guard/ray combinations use the isolation policy so toggling
MAP GUARD or RAYS does not silently re-enable the GI prefilter. Other modes keep
their existing policy. World-albedo guides, projection rays, MAP GUARD OFF default,
restored pre-R184 reflection view, sampling and spotlight filtering match R205.
This is a diagnostic build; reduced ghosting or increased noise must be measured
in-game before adopting the bypass as a fix.

## Test

1. With R205, select Native Blend, RR ON, SPEC Native, hit distance OFF,
   projection rays and MAP GUARD OFF. Disable frame generation.
2. Repeat the carpet movement in the original clip. Record movement and a few
   seconds after stopping. Save an F10 SAVE LOG.
3. Close the game, install R206, and repeat with the same settings and view.
   Let loading and reconstruction settle before recording. Save another log.
4. Report whether the trail disappears, shortens or stays the same, and whether
   stationary lighting becomes noisier. Native is an additional control.

No F9 suite or stationary shadow trace is required. Under active RR the existing
lighting_policy record should show gi_filter=0 and rr_selected=1 in R206 Native
Blend. The build label is R206; unchanged component logs may retain older labels.
If trailing remains, isolate the world-albedo guide next, as a separate change.
