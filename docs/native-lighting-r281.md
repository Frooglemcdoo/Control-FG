# R281: same-dispatch spotlight and complete GI capture

This diagnostic records executed spotlight terms, complete GI rays and final
normalized GI at the same pixels in one dispatch. It leaves native lighting
instructions unchanged. It is not a speckle fix or a sample-reuse implementation.

1. Install beside Control_DX12.exe, preserving folders.
2. With the game closed, run ControlTOH/Diagnostics/Start-GI-Joint-Trace.cmd.
3. Keep the lamp intact, RR and RT indirect ON, Native Blend, FG OFF,
   GI RAW BYPASS and REF CLAMP: BYPASS.
4. Center the crosshair directly on a speckled plant leaf or hair patch. The
   capture covers a dense 32x32 square at the dispatch-grid center. Start with
   the plant; one capture is sufficient for the first analysis. Mouse zoom is OK.
5. Press F7 once, wait for ARMED then SAVED, exit normally and send all ZIPs.
   Do not use CAPTURE RR or PACKAGE RR.

The inherited launcher mentions R278 accounting; the active output is
GI-Joint-R281-PID.bin. Startup logs report GI_JOINT_R281_ROI and export records.
The first header word is record count; remaining words are marker 0x281 and
ROI origin XY. Header metadata, not filename alone, determines interpretation.
The 32x32 ROI is centered on rounded dispatch coverage; with padded dimensions
its center can differ from the rendered viewport center by a few pixels.

Every record uses 256 bytes. Kinds 1/2 preserve ray/summary accounting; kind 3
contains executed spotlight terms with same-pixel/ray material context. The
normalization uses recorded total ray count and depth scale. Skipped branches
have no light record; absence is not a measured zero. Light indices are local to
the session and do not by themselves identify the lamp. Overflow is explicit and
invalidates completeness. The probe does not capture the post-RR image, measure
temporal variance, or prove an invalid estimator. Earlier same-frame image audits
already locate bright samples upstream of RR; this probe targets their lighting
contributions. No instance/triangle readback is enabled by this launcher.
