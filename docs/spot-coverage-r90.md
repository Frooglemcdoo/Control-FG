# R90: retain missing spotlight coverage

R89 improved softness but the user comparison shows missing rail shadows and
a tighter character shadow. The replacement previously forced selected-light
shadow-map visibility to one before applying filtered RT visibility.

R90 preserves native map visibility and merges it with the same selected light's
filtered RT visibility using min(map, RT). It applies that factor once, neutralizing
the later RT multiplier. Equal estimates therefore do not double-darken. Native
material calculations, point-light visibility, R89 rays and filtering remain.

This is explicitly a hybrid coverage repair, not proof that the RT scene now
contains missing geometry. Native map noise or bias can return where maps dominate,
and mismatched estimates can broaden shadows. No additional rays or passes are
added; the native map calculations were already running in R89.

Close Control, install every file beside Control_DX12.exe, keep RT Contact Shadows
enabled, and compare NATIVE SHADOWS / RT SPOTLIGHT using F10. Check the yellow-ended
rail shadows, Jesse's silhouette, softness, speckles and FPS. Save a Lighting-R90
log. No restart between modes is needed.

Validation checks emitted merge instructions across lit, dark and partial coverage,
no double attenuation, unchanged point visibility, shader hashes and Windows WARP
pipeline creation. In-game quality remains to be confirmed.
