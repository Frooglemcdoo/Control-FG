# R164: native PCF-only spotlight blend comparison

## Evidence

R163 completed with zero capture failures: 4096 valid reference rays and 4095 validated map payloads; one ID mismatch was rejected. At all 87 samples for the affected spotlight (capture-local ID 47), native map visibility is 0.68798828125 while filtered RT visibility is zero. The captured spotlight field at byte 200 is 0.31153208017349243. The native shader combines its shadow layers using this factor; the expected fully blocked PCF-only value is 1-factor = 0.6884679198265076, close to the quantized capture. This establishes a missing native blend behavior, not proof of the factor's asset-authoring or distance-fade semantics.

The native shader reads flags at byte 304. Bit 1 enables a separate static VSM layer; bits 8/9 select the other PCF/VSM layer. The affected light has flags 0x101: PCF without that separate static layer. Another sampled PCF light has factor 1; another has factor 0. The accepted RT composition multiplies selected-light radiance directly by filtered RT visibility and bypasses the native map blend.

## Change

NATIVE BLEND is an optional RR-off comparison. Where `(flags & 0x302) == 0x100`, it uses `1 + nativeFactor * (rtVisibility - 1)` for the selected spotlight's diffuse and specular composition. This is the native PCF-only blending formula with RT visibility in place of PCF visibility. It applies per-light metadata, with no room/light-ID exceptions or fixed global fade value.

The correction is deliberately limited to this native layer configuration. Other combinations, including a separate static VSM layer and the VSM replacement path, keep accepted RT behavior. Applying their factor to the combined RT result could weaken unrelated blockers, so this build does not claim to solve static/dynamic separation for those paths.

The change adds five scalar/vector instructions and one private register inside the existing valid selected-light projection branch. It reuses already-loaded flags/factor, with no added texture samples, rays, resources or synchronization. The factor changes shadow contrast, not geometric penumbra width. Actual frame time and appearance require game testing.

Light ranking, nonselected lights, point shadows, authored clipping, geometric normals, centered filtering and current-frame spotlight policy remain unchanged. The accepted DEFAULT remains unchanged. NATIVE BLEND replaces MAP PROBE's overlay shortcut; the internal probe remains available to capture code. MAP PRIORITY remains rejected.

## Test

1. Close Control, install beside Control_DX12.exe, and start a fresh session.
2. From the stairs, compare RR OFF and NATIVE BLEND without moving the camera. Both bypass RR.
3. Record a short video switching, then walking downstairs.
4. Check Jesse, railings, close soft shadows and previously fixed rooms for regressions.
5. Click SAVE LOG and send that log plus the video/screenshots.

Do not use TRACE SHADOW for this comparison: it still selects the accepted RR OFF reference path. No large capture is needed. This is a candidate correction; the false floor shadow is not yet visually confirmed fixed.

## Validation

Independent instruction decoding verifies that the only shader changes are the five inserted instructions and private register declaration. Every previous opcode/operand is retained, including native map samples and point composition. Tests check the flags/factor load offsets and live lanes, all 1024 low flag combinations, fade endpoints, fully lit receivers and the actual captured factor. Windows CI also validates shader disassembly/PSO creation and live mode routing. Package verification compares all existing sidecar and embedded shaders against R163.
