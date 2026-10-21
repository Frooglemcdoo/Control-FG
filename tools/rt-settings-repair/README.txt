Restore the original R71 native shader selection

Close Control. Extract these files beside Control_DX12.exe and renderer.ini.
Run Restore-R71-Shader.cmd. If Windows denies writing to the game folder,
right-click the CMD and select Run as administrator.

The utility prints the current m_iRTSamplesCount value, backs up renderer.ini,
and changes only that number to 1. Use your R71 installation, launch Control,
then select F10 > RT SPOTLIGHT. Save the R71 log and check the blotchy section.

This is a settings repair, not a new rendering DLL. Keep all R71 package files
together; this download contains no replacement DLL or shader.

The count is shared by native RT reflection, diffuse and contact-shadow settings.
Setting it to 1 restores their native single-sample behavior. The mod's separate
GI sample override may still apply. All other saved settings retain their text.
To undo, close Control and copy the printed .bak file back over renderer.ini.

The setting selects the single-sample contact-shadow library when the game first
creates it. A fresh launch after this repair is required. No restarting is needed
between F10 modes after launch. If the utility reports already 1, or R71 still
cannot activate, send renderer.ini and the new R71 log; another override must be
investigated. Missing or ambiguous entries are left unchanged.

The original successful R71 run used 3c6bee7e...; the blocked run used d5efe2c2....
This restores the setting that selects the former. Whether it also removes the
blob remains to be confirmed in game.
