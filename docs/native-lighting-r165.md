# R165: native shadow blending honors the saved RR setting

The user confirmed R164's NATIVE BLEND fixes the reported shadow. R164 intentionally bypassed RR for the comparison. R165 removes that forced bypass from NATIVE BLEND and lets the existing RR evaluation path use the saved user preference.

Enable RR in the game's settings, then select NATIVE BLEND. If RR is saved off, it remains off; this mode does not force-enable it or rewrite the setting. Switching back from an RR-off diagnostic also resumes the saved preference through the existing path.

The R164 native blending shader, layer conditions, rays, centered filtering, clipping, normals and light ranking are unchanged. No shader is added or regenerated differently by this change. DEFAULT remains the previous comparison baseline; select NATIVE BLEND to use the confirmed correction. Promoting the correction to the default is outside this narrow RR-policy update.

The mode's log name is now native_pcf_blend, and its overlay description states that it uses the saved RR setting. RR OFF and the other explicit RR-off diagnostics retain their behavior. TRACE SHADOW still captures the baseline RR-off path and should not be used to verify RR with NATIVE BLEND.

## Check

1. Install with Control closed and start a fresh session.
2. Enable RR in the game settings, then choose NATIVE BLEND in the overlay.
3. Check the corrected stairs shadow and the other affected rooms with RR enabled.
4. Use SAVE LOG after testing if confirmation of the active RR evaluation path is needed.

Local policy checks cover NATIVE BLEND with RR selected and unselected, required bindings, and the unchanged capture fallback. Windows CI validates the full build. Package verification requires every sidecar and embedded shader to match R164 exactly. The R164 visual fix is user-confirmed; its R165 appearance with RR enabled still needs in-game confirmation.
