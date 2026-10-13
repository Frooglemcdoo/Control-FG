Control-FG primary PT M12R7 - active lighting pipeline capture

This is a targeted diagnostic build, not a confirmed PT rendering fix.
R6 completed native writer jobs and recognized opaque/environment passes,
but matched ZERO primary lighting dispatches despite creating the reviewed PSO.

Close Control and replace included files beside Control_DX12.exe.
Keep your existing Streamline installation and game settings.
Load the same scene. F10 opens the overlay: turn PT Reflections ON and play
for about 30 seconds. Leave F11 unchanged. Close Control and run
Collect-PT-Primary-M12R7.cmd, then upload its results ZIP.

The first use of a lighting candidate exports its shader bytes to the log.
A brief first-use logging hitch is possible. No Nsight capture is required.
The capture uses CPU shader bytes; it records no extra GPU readback or wait.
Shader copies are bounded to 12 MiB and released after export.

The original primary shader admission and rendering shaders remain unchanged.
The capture identifies the actually executed shader for ABI review before a
new variant can be admitted. R6's scene-copy memory-budget limit is still
present and separately reported; it has not been bypassed or silently raised.
