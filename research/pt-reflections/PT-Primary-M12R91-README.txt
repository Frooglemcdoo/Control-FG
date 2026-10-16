M12R91 - pending surface camera admission and balanced primary sampling

Surface draws whose recording-time camera upload disagrees with worker TLS can
now retain a stable same-frame candidate. The actual upload must match the
producer camera at submission; wrong views, replay, changed descriptors, rejected
batches and unordered producers still select native lighting. No stale camera
is authorized for GPU use. The log records CAMERA_RESOLVED accepted/rejected.

The full-surface primary continuation now proposes diffuse/glossy equally,
using the matching mixture PDF. The previous unit-color diffuse convention
forced dielectric glossy proposals down to about 5%. This improves glossy
sampling without changing BRDF energy or adding lights. Secondary transport,
four-hit depth, terminal volume GI and fresh RR inputs remain enabled.

This build targets premature surface rejection and starved glossy samples.
Missing baked lighting, secondary material parity and visual flicker are not
claimed solved without an in-game comparison. Sampler root 16 remains required:
the captured native surface shaders really use its material/atmosphere samplers.

Close Control and replace all package files. Keep your current settings; do not
rerun PT-Enable. Run Collect-PT-Primary-M12R91.cmd after the same scene test.
