# R88: restore the selected-spotlight RT baseline

R87 produced poor image quality and unacceptable performance in the user test.
The log confirms the all-spotlight renderer and reconstruction were active.

R88 restores the earlier R71-based selected-spotlight path, using the R79
compatibility handling for both known native shader variants. It retains the
R71 four-sample ray-generation binary, closest-blocker distances, bounded
penumbra filtering, native temporal/spatial filtering, and RR filter handling.

The R80-R87 runtime is removed from the compiled integration: no all-light
frame pools, descriptor tracking, private lighting replay, reconstruction,
or additional queue hooks are installed. Its standalone research sources
remain in the repository, disconnected from the game build. The package does
not contain its runtime shaders; leftover rt_lights_* files are not loaded.

F10 offers NATIVE SHADOWS and RT SPOTLIGHT. This replaces the selected spotlight
per pixel, not all spotlights simultaneously. It is the baseline for further
expansion. Known baseline issues (blotchy regions, coverage/pop-in) are not
claimed fixed by this restoration.

Close Control and copy every packaged file beside Control_DX12.exe. Keep RT
Contact Shadows enabled. Compare modes live with F10; no restart is needed
between modes. SAVE LOG produces a Lighting-R88 log. Check FPS and restored
softness in the same room before further expansion.
