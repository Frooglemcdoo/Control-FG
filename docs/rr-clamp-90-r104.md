# R104: fixed 90% specular clamp

This test changes the current-frame specular clamp from 60% to the requested 90%. Select WORLD ALBEDO with RR on. It retains GI filtering, centered filtered spotlight shadows, textured Standard-material albedo, the existing forced camera-cut behavior, and zero native specular spatial passes. No new spatial-filter experiment is included.

SPEC FILTER remains an older diagnostic that bypasses the clamp. Do not use it for this test: R103 produced increased noise and boiling in that mode. Its log recorded 3,012 validated filter invocations without filter rejection or RR evaluation failure; that confirms the experiment ran, not that it improved the image.

The percentage is defined once in the runtime policy. The shader compiler derives FIREFLY_CLAMP=0.90 from that policy and emits its compiled percentage; the DLL build rejects a shader/policy mismatch. TEMPORAL_WEIGHT remains zero, and the existing ABI checks require temporal-history resources to be compiled out. The clamp formula uses the percentage as an exponent, not an output brightness multiplier.

Close the game to install this ZIP beside Control_DX12.exe, retaining existing NVIDIA runtimes and ControlFGStreamline. Load the affected wall or painting, keep RR on, select WORLD ALBEDO in F10, stand still for ten seconds, then move. Save one Lighting-R104 log and send screenshots. Compare against the same scene and settings in R103 WORLD ALBEDO if needed. No mode-switch restart is required within a build.

The main game log reports requested=90 and effective=90 when the replacement shader is applied. An effective value of 100 means native fallback, not successful application of the requested 90% variant. The generated header is retained in the build artifact for verification that its shader bytes are present in the delivered DLL.

All fourteen spotlight shader assets and the glass shader assets remain unchanged. Visual improvement is unconfirmed until tested in game.
