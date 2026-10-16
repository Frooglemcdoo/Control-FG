# R84 draw-selection observability

R83 results 20261016-114750-517 and video 20261016-1646-09.0808274 show no reported
visual improvement. Log: surface_bindings counter reached 600, 21 sampled guide
submissions accepted, 18 successful RR evaluations, no consumer rejection or
late-expired producer. These counters do not prove which recorded video frames
or objects fell back. Skip counts are draws, not frames, and can include views
outside the main scene. Avoid attributing the full flicker to the 600 count.

Drawing() returns to native rendering before creating a composed draw when
Tracker::Read rejects mask 0x31002. The old reason gave no distinction between
root layout, missing root arguments, alignment and descriptor bounds. Additional
returns for producer/view matching and recording availability were silent.
Neither RR guide success nor producer GPU coverage measures these draw failures.

R84 exposes ReadFailure (reason, parameter, address, requested descriptor count,
heap descriptor count) without changing any acceptance condition. The pointer is
optional and initialized each read. Runtime logs are rate-limited under existing
PT mutex; descriptor description is queried only on rejection. Other silent
post-PSO-selection returns now use existing throttled Reject logging. No new GPU
resources, rays, synchronization or history logic. Added log work can have a small
CPU cost; this is diagnostic and not an FPS comparison build.

Tests use real WARP command-list tracking: injected misaligned CBV -> parameter2,
out-of-range bindless table -> parameter7/count244000, heap switch -> missing
parameter0, then successful read clears diagnostic. Existing valid/reject behavior
remains tested. R84 does not remove guards, hold stale radiance, or claim a fix.

Next decision follows actual cause: repair binding capture/restoration only if
shown stale; repair view/producer association if missing; otherwise inspect GPU
pixel-ownership changes and reflection motion guidance. Do not broaden descriptor
ranges or primary depth tolerances merely to increase coverage.
