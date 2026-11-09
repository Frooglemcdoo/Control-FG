# R68: bounded spotlight shadow history

R67 resolved RR noise in the user test, but shadow flicker and pop-in remain.
The captured temporal shader validates history using screen bounds and depth;
it has no previous spotlight identity input and no local visibility bounds.
R68 clamps reprojected spotlight history to the minimum/maximum current spotlight
visibility in a 3x3 neighborhood before the native blend. Texture coordinates
are clamped at image boundaries. Point history, native depth/motion rejection,
blend weight, spatial filtering, ray generation and lighting composition are retained.
Live native/replacement transitions still use the existing current-only reset pass.

This is a targeted test, not a verified complete flicker fix. Mixed lit/shadow
neighborhoods can still retain stale history. It does not add missing casters,
change selected-light identity, or repair the missing character/railing coverage.
A previous-light ID history would be needed for explicit identity rejection.
The new shader adds eight current-visibility loads per temporal invocation.

Close the game to install all supplied files. Keep native RT Contact Shadows on.
Select RT SPOTLIGHTS; repeat the wall movement and stationary RR comparison.
Save one Lighting-R68 log and send it with the visual result. No restart is needed
between native/replacement comparisons. The rt_spot_history log reports the
configured policy, not GPU completion or visual success.
