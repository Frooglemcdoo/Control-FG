# R83: all-spotlight RT integration test

R83 connects the independent per-light backend to the native lighting pass.
It enumerates the native spotlight list, extracts each light's native material
contribution, traces 32 deterministic emitter samples, and composes diffuse and
specular independently. It replaces neither point nor sun shadows.

Native producer, contact rays and selected-light composition still execute for
fallback. Private replay outputs remove spotlight energy; complete valid RT
contributions are added to those outputs and committed as a pair. Incomplete
receivers retain original native lighting. This avoids adding the new lighting
on top of existing spotlight energy.

GPU scratch memory is bounded and reused between batches. Three recording slots
are prepared on the diagnostic worker, with a total 1536 MiB allocation ceiling.
The render thread does not allocate frame resources, wait for fences, read back
GPU results, or write logs to disk. Slots retire only after actual queue
completion and a successful reset of their recorded command list. Turning the
mode off lets the worker release retired allocations.

## Install and compare

Close Control and copy the complete package beside `Control_DX12.exe`. Preserve
the installed NVIDIA runtimes and `ControlFGStreamline` folder. Use Borderless
and enable native RT Contact Shadows. For the first comparison, disable FG and
glass enhancements, keep the camera fixed, and use F10 to switch between
**NATIVE SHADOWS** and **ALL RT SPOTLIGHTS**. No restart is required between modes.
Initial allocation may take a few seconds; native lighting remains active until
resources and bindings are ready. Then compare RR off/on at the same position.

Save one log with the panel's **SAVE LOG** control. Its filename starts with
`Lighting-R83-`. The `rt_all_pass` lines distinguish resource availability,
private base replay, ray capture, and replacement command recording.
`gpu_confirmed=0` means those lines describe CPU command recording, not a GPU
readback or proof that the shadow problem is visually fixed.

## Limits of this first integration

The standalone DXR test passed on the RTX 5090. Windows WARP checks validate
native extraction, compact work generation, argument layouts, paired commit and
resource retirement. Native replay pipeline creation also passes. These checks
do not establish full native TLAS caster coverage or visual correctness in game.

This version uses deterministic spatial samples without a new temporal denoiser;
native light indices are not assumed stable between frames. High light counts
can exceed a batch's bounded work capacity and retain native pixels. Native
descriptor heap changes, unsupported layouts, a different submission queue, or missing required bindings also retain native output. The first test
must check Jesse's nearby shadow, the blotchy area, wall flicker, RR noise and
frame time. No visual fix is claimed before that test.

R83 carries the captured lighting into composition on a second command list
within the same native pass, queue and descriptor heap. Each recording has its
own reset/submission lifetime; shared resources retire only after both complete.
An unsubmitted consumer restores the resource states at the handoff checkpoint.
The log reports handoff=1 when a second list joins the lease. The supplied R82
log reached this exact boundary on every logged frame but rejected the second list.

The WARP fixture executes a producer/consumer pair, verifies data transfer,
resubmits the consumer and checks that one-list reset cannot retire the pair.
Native queue ordering supplies the producer-before-composition dependency;
unsupported queues/heaps retain native lighting. In-game activation and shadow
quality still need confirmation.
