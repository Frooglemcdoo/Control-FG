M12R8 - captured primary-lighting variant routing fix

R7 executed shader bee42ac0... (69,704 bytes). The old gate recognized only
a9e5b544... (71,144 bytes), so primary lighting never reached PT admission.
R8 recognizes both reviewed variants by exact size and SHA-256. Root-layout,
camera, scene, descriptor and submission checks remain required.

The captured variants have identical input bindings and constant layouts.
The earlier shader additionally writes g_rwtSingleSpot/u2, unused by PT.
The shader fixtures and ABI comparison are included in source validation.

This fixes the confirmed routing mismatch. It does not establish working PT
in-game. R7 also reported geometry-copy budget failures; coverage and remaining
admission failures must be checked in the next run.

Close Control. Extract beside Control_DX12.exe, replacing included files.
Keep the existing Streamline installation. Load the same entrance save with
native RT enabled. F10 opens the overlay: use PT Reflections OFF/ON for about
20 seconds each and note FPS/visual changes. Leave F11 unchanged.
Close the game, run Collect-PT-Primary-M12R8.cmd, upload the results ZIP.
Stop early if performance is unusable or rendering breaks; collect the logs.
