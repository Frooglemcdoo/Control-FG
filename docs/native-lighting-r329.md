# R329: F10 GI recording button

Rendering is unchanged from R328. This build replaces the F10 PERF CAPTURE action
with RECORD GI, which starts an elevated recorder helper and closes the panel.

Launch Start-GI-Owned-Test.cmd, load the problem room, keep RR ON and FG OFF,
select GI RAW BYPASS and GI OWNED, then click RECORD GI. Accept Windows admin
consent. Once WPR starts there is a five-second settling delay. One beep means
stand still for ten seconds; the next beep means slowly move the camera for ten
seconds; two beeps mean you may close the game. Packaging runs automatically
and opens a desktop Control-GI-SEND folder containing ZIPs below 200 MB.

There is no preflight recording, multi-launch comparison, keyboard trigger, or
console input. The selected mode is captured without changing rendering settings.
The runtime log records Owned/RR/FG request state when the button is clicked.
ETW records CPU samples and GPU scheduling, with measured-section QPC markers,
process CPU samples, foreground PID, driver information and matching game logs.
This does not provide per-shader GPU duration or a tracking-disabled baseline.
A comparison can be captured separately using the same button if needed later.

Only the named recorder instance is saved. No unrelated recording is cancelled.
The final save skips managed .NET PDB generation, prints progress, and runs only
after measurement. If the game stays open, it saves after three minutes rather
than recording indefinitely. A failed capture is packaged as partial when possible.
