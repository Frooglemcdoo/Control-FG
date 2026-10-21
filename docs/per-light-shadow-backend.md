# Per-light shadow replacement: implementation checkpoint

User correction: the blotch was present in the original R71 video. Shadow sample
count and shader-variant changes do not explain its origin. Do not ask the user
to run the R71 settings repair for this defect.

## Implemented

- Shared C++/HLSL layouts for receiver/light tasks, keyed visibility, unshadowed
  per-light contributions and receiver output ranges.
- Independent DXR visibility per task, full bounded receiver-to-emitter segments,
  closest-blocker distance, deterministic disk sampling and native-compatible
  float-distance hit payload. No strongest-light selection is performed.
- Composition sums each light's own diffuse/specular contribution multiplied by
  its own visibility. It never multiplies the combined lighting buffer by a
  single shadow. Missing/stale/mismatched results invalidate the receiver output.
- Standalone hardware DXR fixture builds a two-geometry BLAS and TLAS, uses
  separate opaque/cutout hit records, dispatches the actual visibility shader,
  executes actual GPU composition, then checks readback against known geometry.
  It does not modify or inject into Control.

The fixture's alpha handler is deliberately synthetic and excluded from the
production compile. Control's alpha material handling remains an adapter task.
This checkpoint is not hooked into the game and is not a visual fix release.

## Native adapter progress

Hardware Test 1 passed on the user's RTX 5090. Native spotlight BRDF extraction
and frame-keyed contribution packing now pass an executing Windows WARP fixture.
See [per-light-native-extraction.md](per-light-native-extraction.md) for the atlas
contract, preserved native calculations, validation and integration requirements.
Native cell enumeration, world-space shadow tasks, and batch composition also
pass executing WARP fixtures. See [per-light-native-batches.md](per-light-native-batches.md).
The native-binding ray library compiles and the captured root accepts the extraction
compute pipeline. Native DXR linking/dispatch is not yet verified. These components
are not wired into the live game renderer yet.

## Native adapter still required

The captured a9e5b544... producer enumerates lighting before the existing selected
spotlight ray dispatch and writes one selected spot/point ID per pixel. The
6af3f740... composition shader consumes one filtered spotlight visibility channel.
Neither existing storage nor the dispatch order can supply independent visibility
for all lights. A replacement must introduce a producer before per-light lighting
composition, not append another full-screen multiplier to the old pipeline.

1. Extract/cull every contributing spotlight into complete batches, with stable
   frame/light/receiver identity. If a batch exceeds capacity, retain native
   rendering for the affected receiver; never silently drop lights.
2. Verify that the TLAS contains the needed off-screen and animated shadow casters
   and that its build/update has completed on the consuming GPU queue.
3. Bind native alpha-tested geometry records to the new float-distance payload.
4. Preserve native light color, cone, range, projection/pattern and BRDF values in
   each unshadowed contribution. These must be captured before map occlusion.
5. Filter visibility per light with depth/normal/motion guides and stable identity.
   The old selected-light history cannot be reused as an all-light history.
6. Replace corresponding native map/contact contributions only after the complete
   batch is valid. Resource and descriptor retirement must be fence-driven; no
   synchronous render-thread readback, file writes or waits.
7. Integrate F10 live switching with history invalidation, then test the affected
   room. Point lights follow after complete spotlights are validated.

## Hardware gate

Windows CI compiles the fixture and both shaders. It has no hardware DXR adapter
for this test. Run Run-Per-Light-Test.cmd on the user's RTX GPU and inspect the
single log. A PASS proves only the synthetic backend cases, not native integration
or the absence of the in-game blotch. A failure must be resolved before attaching
this backend to Control.

References used for the design:
- NVIDIA NRD README identifies SIGMA as per-light shadow filtering and supports
  local spot/omni sources: https://github.com/NVIDIA-RTX/NRD/blob/master/README.md
- Microsoft DXR specification defines shader-table addressing, resource binding,
  instance masks and acceleration-structure synchronization:
  https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html
