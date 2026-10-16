M12R88 - separate terminal GI callable shader

GI remains enabled. Its volume lookup is now a separate DXR callable shader,
invoked once after the tracing loop finishes, with the same terminal position,
normal and throughput. This removes the GI tree walker from the ray-generation
shader compiled around TraceRay. No new rays, full-screen passes or per-pixel
buffers. The callable adds64 bytes to each shader table and incurs scheduling
and stack overhead; performance is not yet measured.

The R87 capture shows device removal during PT pipeline initialization before
any private PT dispatch was submitted. It cannot show a GI traversal GPU fault
because that work did not run. This build targets the changed pipeline structure,
but a driver/compiler cause is not proven. Each creation stage now reports the
removed-device reason before/after failure, plus a successful private-pipeline
marker, so another failure can be localized instead of collapsed into one HRESULT.

Validation: shader compilation and callable export/call inspection, callable-table
alignment/bounds tests, GI arithmetic on WARP, and existing regression gates.
The CI host cannot validate NVIDIA DXR pipeline creation or in-game appearance.
Existing missing-light/reflection/flicker limitations remain under investigation.

Close Control and replace ALL package files while preserving settings.
Do not rerun PT-Enable. Use Collect-PT-Primary-M12R88.cmd after gameplay.
