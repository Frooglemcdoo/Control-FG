# R31: live GI sample controls

User confirmed R30/four samples removes almost all noise on both blinds and Jesse, and requested six/eight with live buttons.

F10 -> RR settings cog now exposes Native, 2, 4, 6, 8. Default is four. The normal overlay settings file stores RayReconstruction/GISamples. The old comparison INI is no longer read or packaged. Existing settings are not replaced.

The UI writes an atomic request only. The render-thread frame-reset hook latches it immediately after RRFrameCoordinator::Begin selects a full-RR frame, before native GI allocation or shader binding. The producer getter reads only this snapshot for the entire frame. A changed snapshot marks RR history for a single reset, propagated through the existing RR evaluation reset path. Rapid clicks coalesce at frame boundaries. Off/partial frames retain native sampling. Unknown native counts other than one remain untouched.

Native hit-buffer setup at renderer+0xa2cd0 already calls +0xa3190 to return previous buffers through the native pool (+0xa9e00), then obtains descriptors for the current count. R31 retains those engine ownership operations and does not allocate/release resources on the UI thread. No manual GPU resource destruction or extra queue flush is added. GPU lifetime and pool reclamation remain engine-owned; immediate VRAM reduction after switching down is not promised.

Tests exercise 2/4/6/8, rejected values, native-count preservation, UI requests during a frame, live sequence 4->4->6->8->2->Native->4, one-shot reset, and rejection of late/off-frame resets. The clean-baseline verifier reverses exact UI/coordinator changes through r31-baseline-exceptions.json before comparing original hashes. No baseline hash is replaced. Windows CI retains shader, hair/skin, and capture checks.

Six/eight-sample quality and live resource transitions still require in-game testing. Four remains the user-confirmed initial selection. Guide/shader bytes, GI radius, glass and PT code are unchanged.
