# R275 — visible GI trace confirmation

This build changes capture control and feedback. The R274 shader probe and
native lighting arithmetic are unchanged. The previous run initialized the
probe but recorded no accepted F7 trigger; the cause is not established.

## Capture

1. Extract beside Control_DX12.exe and run
   `ControlTOH\Diagnostics\Start-GI-Spot-Trace.cmd` with Control closed.
2. Use GI RAW BYPASS, RR ON, RT indirect ON, Native Blend, FG OFF. Keep the lamp intact.
3. Frame the plant and Jesse's hair. Press F7, or open F10 and click **ARM GI TRACE**.
4. Look for the **ARMED** five-second countdown in the bottom-right banner.
   Close F10 and hold your mouse zoom. Keep the view steady.
5. Wait for **SAVED**, then exit normally. The launcher packages the trace and logs.

The banner stays visible with F10 closed. It is non-activating and passes mouse
input through to the game. Mouse buttons do not block arming or the countdown.
F7 input is checked every 50 ms instead of during the 500 ms log drain.
Ctrl+Shift+F7 remains reserved for the existing character tool; a rejection is
shown. F7 logs focus, modifiers, mouse-button state, readiness and capture phase.

If the banner says **WAITING** or **FAILED**, send its text and the generated ZIPs.
Do not keep repeating captures. Failure reasons that were previously silent are
now logged and displayed. Recorded/submitted/saving states are separate from
SAVED; success is shown only after the GPU fence completes and the file is written.

One capture is permitted per game process. No full RR image capture is needed.
The bounded R274 trace format remains unchanged: one sampled GI dispatch,
65,536 records maximum, 256-byte records, explicit stage validity and overflow.
This remains a diagnostic build, not a speckle fix.
