# R172 — shorten serial SPEC material lifetimes

This is a new optimization candidate on top of the R171 revert, not another rollback and not a verified performance improvement.

The captured native shader loads material values into r23.xyzw and r24.xyz at instructions 405–406. Those seven scalar components remain live across 536 intervening static instructions, including point and spot light loops, before use at the final material composition. R172 moves the same two load instructions immediately before instruction 943. It does not remove any instruction, approximate lighting, change sample count, reorder accumulation, change the clamp, change dispatch dimensions, or add synchronization. GI and skin/hair guide shaders are unchanged.

The move stays inside the same enclosing branch and sample loop. The material index r5.w is unchanged across the interval; no intervening instruction reads/writes the moved destination components, and the material SRV is read-only. Both loads execute the same number of times. r24.w remains untouched by the moved xyz load. Existing point/spot loops, textures and shadow taps remain intact.

This shortens source-level live ranges. Actual GPU register allocation, occupancy, memory latency and FPS may improve, remain unchanged, or regress. The driver may already perform the same scheduling; delaying loads can also expose latency. No performance claim follows from static instruction counts.

## Integration and validation

Only the exact native SHA256 4fc0a0e9cd3706bcb86e29444d86fa32616138e1712c53b1b5103b30d7a1ecc9 is accepted. The generated SHA256 is c4bebcb782c972985e77ea22c04dd8301832b36f863379f3f041a1801ed02889. The shader is substituted once at compute PSO creation; incompatible shaders stay native, and failed replacement PSO creation falls back to native. No per-bind metadata or parallel dispatch path is introduced. The transformation preserves all sample counts; it also applies to SPEC Native when that same shader is used.

The local audit checks binary operands, control-flow scope, destination/address hazards, unknown-input rejection, and exact recovery of the original instruction stream by undoing the two-load move. Windows CI additionally validates reflection, root-signature compatibility and WARP PSO creation. These checks do not replace on-device output comparison or measurement. The old parallel fixtures do not validate this optimization.

The pipeline timing hash remains the source identity. The lighting runtime log reports SPEC_SERIAL_LIFETIMES active=1 when replacement PSO creation succeeds; that is creation evidence, not GPU-execution proof. Missing/active=0 means this candidate is not confirmed active.

## Comparison

Compare R171 and R172 in separate fresh sessions at SPEC 16, GI 6, NATIVE BLEND, RR on and FG off, with identical resolution and view. Use PERF CAPTURE for each. Inspect stationary and moving reflections, paintings, glass, hair and small lighting details. A SPEC Native capture is a secondary scaling check; it is not an unmodified-shader A/B because R172's replacement also applies there.

Keep the change only if repeated whole-frame and target-compute measurements improve without visual regressions. R171 remains the reference. No downloadable Windows build or in-game validation has been produced locally.
