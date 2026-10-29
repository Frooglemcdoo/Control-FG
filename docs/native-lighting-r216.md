# R216: GI boundary capture

This diagnostic leaves displayed lighting unchanged while capturing the existing RR boundary buffers. It does not attempt another ghosting fix. Rendering defaults remain Clamp Only, Cleanup Off, View Normal; R215 shader variants are unchanged.

1. Restart with this build. Use Native Blend, RR ON, FG OFF, SPEC Native, hit distance OFF. Stay at the projector-room view where speckles are visible.
2. Keep GI VIEW NORMAL and CLEANUP OFF. Select GI CLAMP ONLY, settle for 10 seconds, then F10 > CAPTURE RR (or F9). Hold the camera still until the panel reports the pair saved.
3. At the same camera position select GI BOTH, settle for 10 seconds, then CAPTURE RR again. Wait for both pairs saved.
4. Click PACKAGE RR. Send all generated ZIP parts from the Desktop GI-Boundary-R216 folder. If capture fails, PACKAGE RR retains logs and available metadata; send those instead.

Capture records two successive evaluated frames per mode: GI boundary texture, accumulated diffuse/specular lighting, scene color, material guides, depth/motion, and RR output. Existing readback copies restore resource states; no GPU contents or history are cleared. Settings are locked while a pair is pending. Resolution limits and state validation remain those of the existing capture path.

Metadata records the applied filter stage, view, cleanup, spatial pass count and frame. The native filter's output color resource identity is observed after the original call returns, only while a capture is pending. Identities are compared to captured resources from that same frame; stale observations are rejected. No-match is retained as evidence, not treated as proof of GI isolation. `raw-gi.rgba16f` is a historical filename: at this boundary it may already contain filtering. `diffuse.rgba16f` and `specular.rgba16f` are RR material guides, not accumulated lighting.

Native trace: renderer RVA 0x12cd7d takes the slot at 0x90e9f8; helper 0x133a0 dereferences it before filter 0x14c90 receives the color container. The color argument is a NativeTextureContainer and is resolved with its own const getNativeTexture export (renderer IAT 0x5dfd30), not the ShaderTexture getter used for captured globals. The filter receives history slot 0x90eac8, skips spatial passes when their count is zero, and copies its final output back to the color container (copy call at 0x15b03). These addresses establish the write-back, not the downstream lighting composition. R215's too-dark Zero view is therefore inconclusive about the ghosting source. This capture is intended to resolve that ambiguity.

Production branch is unchanged. In-game validation still requires the captured buffers; CI checks do not establish visual quality.
