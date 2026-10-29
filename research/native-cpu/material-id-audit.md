# Native CPU material-table audit

Scope: existing 2026-10-29 base and modded ETL recordings. No new capture,
runtime patch, or change to the accepted R217 visual settings.

## Identity and measured work

The local renderer DLL's CodeView GUID `E1544783-5673-4712-A071-7203173304D1`,
age 1, appears in both traces' image-identification records. Renderer image
size is `0x1306000`. Addresses below are RVAs for that build only.

The busiest game threads were 89704 (base) and 90208 (modded), with 29,060
and 29,146 CPU samples in the selected gameplay windows. Own samples:

| Routine | Base | Modded |
| --- | ---: | ---: |
| renderer + 0xC7BC0 | 1,312 (4.51%) | 1,185 (4.07%) |
| renderer + 0x1BF4E0 | 1,819 (6.26%) | 1,698 (5.83%) |

The caller at `0x12490E` opens the game's `UpdateMaterialIDManager` marker,
calls `0xC7BC0` at `0x12491D`, and closes the marker at `0x124930`.
The second routine performs shader/material selection and rendering-data
preparation; its exact original function name has not been established.

Observed stacks place the material-table routine in 3,128 base samples
(10.76%, including callees). These are sample proportions on one thread,
not whole-machine CPU percentages, frame times, call counts, or guaranteed
FPS improvements. Stack recovery is partial; preserve the own-sample
measurements as the more directly reproducible comparison.

## Existing safeguards: do not bypass

1. `0xC7CA7` checks manager byte `+0x118`. If clear, the rebuild is skipped.
   `0xC838A` clears it after rebuilding.
2. `0xC7A20` calls the material hash routine `0xC4690`. At `0xC7A66`, the
   computed hash is compared with the stored hash; equality skips the
   copy and dirty write at `0xC7B96`.
3. The hash includes an eight-byte value, 0x150 bytes of material data,
   and 35 texture pointers when the texture-slot structure is present.
4. `0xC7BB0` is a separate invalidation callback. `0xC9A10` registers it
   with the texture-related object via `0x156F60`; `0xC99D0` removes the
   registration via `0x157060`. These paths can dirty the manager without
   a changed material hash. Do not infer identical texture pointers mean
   identical current native resources.
5. Entry creation/removal paths also set the manager dirty. During rebuild,
   entries are sorted/deduplicated and their assigned IDs rewritten.
   A selective-update implementation must preserve these mappings.

Thus the simple proposal to skip unchanged material submissions duplicates
an existing hash guard. Suppressing the global dirty flag is not justified.
The current evidence does not identify which invalidator dominates at runtime.

## What the hot loop actually does

For each resulting unique entry, the rebuild uploads eight bytes and a
0x150-byte material record, then resolves 35 texture slots. The loop at
`0xC8122..0xC81AD` follows the selected texture resource, obtains the native
texture and its descriptor, substitutes the null descriptor when needed,
and stores the descriptor at the entry's current output index.

Base own samples are concentrated at `0xC8125` (478), `0xC8145` (325), and
`0xC8131` (216). These locations implicate the pointer-resolution loop;
instruction sampling alone does not prove cache misses or memory bandwidth
saturation.

The imported `GlobalDescriptorManager::setMaterialDescriptor` in the local
D3D DLL is only a handle store plus a version-counter increment:

    mov [rcx + rdx*8 + 0x1F8], r8
    inc qword ptr [rcx + 8]

It is not itself a D3D12 descriptor-copy call. Optimizing just that setter
does not remove the hot texture-resolution work. Persistent descriptor
caching would need explicit resource lifetime and streaming invalidation.

## Other significant CPU work

The D3D submission routine at `0x434A0` collects pending command lists and
calls through its queue at `0x4355A`. The return address `0x4355D` appears
in 3,726 base busy-thread sample stacks (12.82%). Much of this inclusive
work is below the game wrapper in the driver/runtime. Queue-fence operations
are separate calls later in the routine; do not describe this entire path
as CPU waiting for the GPU.

## Decision

Retain the current runtime behavior. The material-table rebuild and command
submission are concrete, independent targets. Further work should establish
the dominant invalidation path and safe opportunities to reduce repeated
texture resolution within a rebuild. No evidence yet supports a persistent
cache, dirty-flag suppression, or a claimed performance gain. Existing ETLs
remain useful for further caller attribution; another broad capture is not
a prerequisite.
