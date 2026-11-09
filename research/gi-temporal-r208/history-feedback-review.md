# Both-mode ghosting: existing evidence review

Reviewed 2026-10-31 against the R258 source. This is a source/evidence audit,
not a rendering change or a demonstrated ghosting fix.

## Established observations

R213 user testing: Both suppresses noise and speckles but ghosts; Spatial Only
suppresses speckles but boils; Clamp Only avoids ghosting with residual speckles.
R210/R211 increased motion rejection from 40 to 400 and reduced relative depth
tolerance from 3% to 1%. R212 added history RGB clipping. Neither established a
visible ghosting fix. These same adaptive changes remain selected for Both.
Do not repeat these threshold experiments as if they were new.

The R216 and R257 boundary image pairs are stationary samples. They establish
filter application and input variation, not where a moving trail first appears.
The foliage guide gap is separately confirmed and is not a demonstrated cause
of the temporal artifact. Coverage expansion is paused for this investigation.

## Native feedback chain

Source: `research/native-integration-audit/evidence/diffuse-filter.asm`.

- Function entry saves color, history-slot pointer and spatial count. With the
  established frame layout they are `[rbp+0x1c0]`, `[rbp+0x1c8]`, `[rbp+0x1d0]`.
- Temporal dispatch is at renderer RVA `0x156d3`. Afterwards r14 is the temporal
  output container and r12 is the alternate container.
- Spatial dispatch is at `0x15a28`. The loop swaps r12/r14 at `0x15a2e..0x15a34`.
  Thus r14 names the last spatial output after the loop (or temporal output when
  the spatial count is zero).
- At `0x15a58..0x15aa3`, the native code dereferences the saved history-slot
  pointer, resolves both native textures, and copies r14's texture into history.
- The subsequent copy at `0x15b03` writes the result back to the original color
  container when required.

Consequently Both feeds its spatially filtered output back into temporal history
on the next frame. This is native behavior, not evidence of an erroneous mod
copy. It is a relevant mechanism to test: spatially spread lighting can become
accepted temporal history. It does not by itself prove the observed cause.

## Validation gap

`tools/gi_history_gpu_fixture.h` executes the shipped temporal bytecode, but its
history texture is independently initialized for each case. Velocity is zero
in every case; dimensions are equal for current/history coordinates. It does
not feed the spatial result into the next frame, move an occluder, or exercise
nonzero reprojection. Passing those tests cannot validate moving history.

Next implementation work should extend the actual-bytecode GPU fixture to
nonzero motion and changing depth/illumination, then model the verified native
feedback chain with the actual spatial shaders once recovered. Keep motion of
geometry separate from illumination moving across stationary geometry. If this
does not reproduce the trail, a narrowly scoped temporal/spatial/RR boundary
motion capture is justified; repeating the four-mode visual comparison is not.

Do not remove spatial passes, clear history each frame, or alter the accepted
Clamp Only smoother as a speculative fix. Preserve Both as the visual reference
until a separately selectable correction has numerical and runtime evidence.

Local checks passed: `python tools/test-gi-motion-history.py` and
`python tools/test-gi-temporal-audit.py`. These are bytecode/source checks, not
new Windows GPU or in-game validation.

## R259 follow-up: second temporal blend recovered

The loose-shader archive is available locally. Its two 12212-byte spatial variants
contain their own 70% final-pass history blend (SHEX DWORDs 1896–2173). This
corrects the earlier assumption that only the first dispatch uses history. The
R259 test isolates this second blend; see `docs/native-lighting-r259.md`. Moving
the history copy ahead of the spatial passes would change their history input
and is not the chosen experiment.
