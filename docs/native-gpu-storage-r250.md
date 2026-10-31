# R250: measure GPU-ready to native renderer publication

R247 remains the accepted baseline. R250 adds an optional diagnostic observer;
it does not alter texture demand, transfer lanes, priorities, budgets, publication
order, resource states, shaders, or the game's native completion routine.

The Steam renderer hash is EBFB5B47CEBC2D4482E912B8090BD343F717BAB8A178B5A6385DEC9105E7C433.
Static inspection identifies consumer 0x158410. Its success path updates the mip
alias/best level and writes state 2 at 0x15849D, before native notifications.
Three verified incoming branches are 0x1603B9 (E8), 0x1605D0 (E9), 0x162B74 (E9).
The last is the deferred function-object callback, preceded by RCX+=8. Hooking
the tail branch after this adjustment preserves the consumer's record argument.
The existing call-patch helper now explicitly supports E8 or E9; the default
remains E8. Windows executable fixtures verify tail forwarding and rollback.

Immediately before successful finishLoadLevel dispatch, record texture identity,
mip, container identity and a timestamp. On native consumer entry, match and
remove the record, then invoke the original consumer exactly once. Record the
handoff delay and consumer-call duration. No texture fields are accessed after
the original consumer returns. LastError is preserved in both directions.

The tracker stores integer identity metadata only, with 4096 slots and eight
probes maximum. Try-lock contention and full probe ranges drop observations,
never delay native completion. Entries expire after 30 seconds; replacement
container identities do not match. A new observation for the same texture/mip
replaces the earlier identity. Native pointer reuse/cancellation is not fully
modeled; these are diagnostic samples, not a complete lifecycle ledger.
No extra engine/COM ownership is retained by this observer.

Run Start-CPU-R250-NativeGPUStorage-4ms.cmd for one normal traversal. If texture
detail visibly arrives late, note the location. Send compact logs and whether
frame pacing stayed smooth. The launcher enables CONTROLFG_STORAGE_PUBLICATION=1.
Setting it to 0 disables this observer. Do not enable the older texture-contract
or loader-trace experiments alongside it. No R246 trace or Reference run needed.

Expected: DIRECTSTORAGE_R250_PUBLICATION installed=1 sites=3, followed by tracked
and matched counts. New stages gpu_ready_to_renderer_publication and
renderer_publication_call are cumulative CPU wall timings. The first ends at
consumer ENTRY, before state updates; the second includes native consumer work
and a small observer overhead. Neither measures first visible texture use or
upstream native demand. Mismatches, expiry, contention and native-only callbacks
can make tracked and matched counts differ. Inspect coverage before drawing
conclusions; zero samples are not evidence of zero delay.

Existing accepted scheduling and prewarm tests, 72-mip content checks and R248
lifetime tests remain. New tests cover identity matching, replacement, expiration,
bounded collisions, single consumption, native forwarding and LastError.
This is a measurement build; no texture-speed or FPS improvement is claimed.
