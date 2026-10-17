M12R102 - authored direct-light shadow policy

Point and spot lights now honor the native per-light shadow enable flags.
Previously PT forced a shadow ray on every source, including authored fill
lights that the game deliberately evaluates without shadows. Geometry around
such a source could remove its entire diffuse/specular contribution, both on
primary surfaces and on objects seen in reflections.

Shadow-enabled sources retain traced occlusion. Geometric hemisphere rejection,
native attenuation/projection, R101 light radius and R100 baked diffuse lighting
remain. No global brightness boost or blanket shadow disable is applied.

Close Control and replace package files in the SAME installation. Keep settings;
do not rerun PT-Enable. Compare reception wall cones, the planter floor and lobby
reflections. Run Collect-PT-Primary-M12R102.cmd and send the results ZIP.

Windows/GPU tests validate decoding and policy; visual confirmation is pending.
This does not establish that all missing lighting/reflections are resolved.
