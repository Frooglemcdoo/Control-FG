M12R81 - correct PT-to-RR guide submission timing

R80 logs showed bound=0 for every sampled PT diffuse guide. The matching source
required producerSubmitted/armed during guide command recording. Native recording
precedes queue submission and late COPY-fence admission, so this rejected valid
current-frame producers. R80's shader test passed but did not test this scheduling.

R81 records an owned exact-frame source plus an initially disabled GPU guard.
At queue submission, the guard is enabled only after an earlier producer list
has been armed or accepted as COPY-fence-deferred. The shader checks this guard
before reading PT buffers, then checks pixel coverage and material identity.
Missing/unordered/cancelled/replayed/wrong-queue/scene-rejected work falls back.
Foreign-queue rejection waits for the owning-queue clear. Same-batch duplicate
consumer lists are rejected too. Existing source/guard lifetime protections stay.

RR_PT_DIFFUSE_GUIDE bound=1 means source recorded, not yet admitted.
RR_PT_GUIDE_SUBMIT accepted=1 and guard_ok=1 confirm ordered submission.
Deferred admission still depends on the existing late COPY signal and zero
coverage on a rejected producer. No new CPU wait, ray or spatial-filter pass.

WARP tests execute the real guide shader with disabled/enabled/rejected guards,
check native fallback, validate submission policy, and verify a foreign queue
waits for guard invalidation. Existing runtime/lifetime/settings tests remain.
In-game binding and visual quality still require capture verification.

Missing lighting is not fixed here: R80 reports environment=0, and the current
light sampler covers point/spot/sun delta sources, not area/emissive-light NEE.
Optional specular-hit-distance integration is also unfinished.

Close Control and replace ALL package files while preserving settings.
Do not rerun PT-Enable. Collect with Collect-PT-Primary-M12R81.cmd.
