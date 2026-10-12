# G2D5: measure diffuse survival and reflected shadows

G2D4 user result: noise fixed, no visible Glossy versus Glossy + Diffuse difference, roughly 10 FPS cost. CPU `history_previous` proves a retired slot exists, not that any pixel accepts it. G2D5 adds observations without changing estimator weights or shadow attenuation.

## Capture contract

Every 120th glass recording, a 32-pixel grid writes 128-byte records to a dedicated u2 buffer. Layers 0/1/2 are first reflected surface/glossy continuation/diffuse continuation. Each selected pixel initializes its record before native miss or secondary-hit rejection and marks successful shading at the output store. Integer epoch and mode reject stale records. Secondary and composite use disjoint layers. The consumer transitions and copies the buffer to READBACK after both dispatches; mapping occurs only after the resource pool has observed actual producer and consumer completion. Discarded recordings invalidate pending diagnostics. No GPU wait is added.

`PT_GLASS_GPU` reports eligible diffuse pixels, accepted/rejected history, count >= 8 warmup, and pixels whose material-weighted absolute RGB change exceeds 1% of final reflected luminance. Means are divided by eligible diffuse pixels. Native/raw/spatial/resolved values are luminance before material response. `material_delta` is signed luminance; `material_abs_delta` is luminance of absolute RGB change. The delta follows the actual r12 and r4 material factors, including emission in the final-light comparison denominator. This isolates this dispatch's diffuse change relative to the same glossy path, before later native reflection composition/denoising. It is not a final-screen image difference or a GPU timing measurement.

## Shadow audit

The hash-locked native shader already evaluates shadow maps at the reflected hit, and the same lighting instructions are reused for extra hits:

- Point light flags at native dword 3231: flag 0x2 enables point VSM. Atlas t5 is sampled at 3744; visibility is finalized at 3828 and multiplies light RGB at 3837. G2D5 observes immediately before 3837, plus lights considered at 3291.
- Spotlight shadow flags at 4885: flag 0x2 and mask 0x300 select shadow paths. t8 VSM is sampled at 4917/5558; t7 PCF has eight depth comparisons at 5098 through 5517. Combined visibility is selected at 5735. G2D5 observes it at 5744, before the native fully-shadowed-light early branch at 5751.
- `point_lights` counts point-light candidates reaching the shadow flag branch; `point_shadow_tests` counts enabled point VSM evaluations. Point mean visibility uses tested lights only.
- `spot_lights` counts spotlights reaching the combined shadow result; `spot_shadow_eligible` counts flags requesting shadow maps. Spotlight mean visibility includes unshadowed candidates. Occluded means visibility < 0.99, not a count of shadowed pixels, and is not energy weighted.

Native world-to-view transforms and light-cell selection remain intact. Atlas availability, caster inclusion, update/culling behavior, and ray-traced contact-shadow visibility cannot be proven from CPU activation logs or from the presence of shader instructions. This build does not add a new shadow-ray path or missing casters. Actual scene captures are needed to choose between atlas/caster issues and attenuation/composition issues. Sparse samples can miss narrow shadows; screenshots locate the affected area.

## Validation

The temporal WARP fixture executes production diagnostic stores and temporal accumulation, then checks integer identities, 128-byte layout, separate record addressing, real fence readback, per-pixel acceptance/warmup, and CPU shadow-summary decoding. The native-root PSO gate checks u2 in both secondary and composite. Resource-pool tests cover allocation sizes, discard invalidation and blocked-fence lifetime. All 871 captured native instructions remain emitted, and original opaque runtime hash gates remain mandatory.
