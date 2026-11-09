# R209 — GI compute identity and RR fallback

Experimental continuation of R208 on the accepted R205 baseline.

R208 scanned descriptor slots 0x10/30/50/70/90 and missed compute at 0x60. Its rejection path latched RR off. The supplied test recorded zero successful RR evaluations; it was not a valid ghosting test.

R209 reads only compute slot 0x60, requires stage 5, 5076 bytes and the audited diffuse temporal SHA-256. The native renderer SHA ebfb5b47cebc2d4482e912b8090bd343f717bab8a178b5a6385dec9105e7c433 binder at 0x1db750 copies seven slots at 0x10 strides; the checked-in disassembly records that evidence. Existing composite shader code independently uses 0x60/stage 5.

The override sets CameraCut only across GI temporal bind/dispatch, preserves the shader's current-frame energy clamp and native spatial passes, then restores the provider. There is no history clear. Unknown shaders and rejected provider setup fall back to native GI filtering with RR selected. Provider snapshots survive failed write verification; an unverified restoration still stops RR as a genuine integrity failure.

First four and every 240 calls report applied, reason, restoration, dispatch count and RR selection; initial descriptor reports include stage, identifier and byte count. Success requires applied=1, dispatches=1, restored=1 plus successful RR evaluations in the session. Tests cover the compute-only descriptor, hash rejection, missing descriptor, provider rejection, partial-write restoration, exceptions, restoration failure and inactive modes.

Test the same Jesse movement and reflective floor with RR enabled in Native Blend. Check both motion trails and speckle suppression. Visual improvement is unverified until an in-game capture. Accepted R205 projection rays, MAP GUARD OFF, angled reflections and default settings remain unchanged. No production branch changes.
