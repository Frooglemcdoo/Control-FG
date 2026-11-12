# RT replacement reset — user authorized 2026-11-12

## Decision

R431 successfully recorded guided completions but user reports persistent
speckles and severe performance loss. Reject it as a product direction.
Do not ship another denoising/ray-count/guiding variation without distinct
source-backed evidence. Do not request previous scene-order captures again.

## Scope

Assess ownership of the complete RT subsystem: scene representation and
updates, acceleration structures, hit/material evaluation, indirect lighting,
reflections, RT shadows, and reconstruction inputs. Preserve the game as the
source of animation, geometry, materials, lights and visibility updates.
This is a renderer-subsystem replacement, not a promise of engine-source access.

## Verified constraints from current implementation

The existing guided method borrows native scene/heap lifetimes, snapshots
hit-source geometry/SBT/constants per invocation, gates native preparation,
and delays a portion of tracing to GI shading. It does not own the whole RT
scene or replace all native AS preparation. That architecture's costs cannot
be assumed to disappear by substituting another tracer.
Native ray/GI6 suppression exists for the guided invocation; whole-subsystem
suppression and replacement coverage have NOT been established.
The role of RR inputs versus raw shading in the remaining character artifacts
is unresolved; a new tracer alone is not evidence of a noise fix.

## First engineering deliverable

A source-backed ownership/suppression map of the native RT passes, supported
update interfaces, and unknowns, using existing binaries, captures and logs.
For each producer list its output consumers, lifetime, dynamic update source,
replacement entry point and precise native work that can be suppressed.
Identify whether persistent geometry/AS ownership can avoid per-frame full
geometry copying without unsafe resource borrowing. Do not assume it can.
Inventory the native material/light evaluation contract before choosing a
replacement integrator or new SDK. Define RR inputs from actual rendered
surfaces, including animated skin and alpha-tested hair.

## Acceptance gates

A representative vertical slice must replace its corresponding native work,
prove output reaches the final frame, preserve dynamic scene/material behavior,
and have measurable CPU/GPU cost. No claimed performance benefit without
measurement; no large full-scene build before these conditions are met.
A complete replacement remains conditional on feasible scene access and cost.
Next user build must have a concrete demonstrated purpose beyond another
parameter trial. No user test requested by this document.
