M12R53 - fix PT selection falsely reporting save failure

Close Control and extract all package files beside the installed dxgi.dll.
Open the overlay, select PT ON (RESTART), then fully close and restart Control.
The saved selection persists through updates. No enable script is needed.
Native RT must be enabled for PT rendering.

R52 incorrectly treated a zero return from the INI cache flush as a failed
write. Windows documents zero as a possible result of flushing the cache.
The overlay could therefore retain its old selection and later overwrite the
requested value. R53 verifies the saved key after flushing; actual write and
readback failures remain errors. The optional enable script uses the same fix.
Save logs now distinguish path, write and readback failures. Settings paths also
use normal single separators matching the existing overlay settings path.

PT remains restart-only. PT OFF followed by restart bypasses capture hooks.
This package changes no shaders and makes no new performance claim.
It includes no renderer.ini and does not reset native RT, DLSS, HDR or quality.
The cause of the earlier native game-settings reset remains unestablished.

Windows validation includes a regression that forces a zero cache-flush return,
checks persisted on/off selections and unchanged startup state, preserves other
preferences, and rejects read-only writes. In-game confirmation remains needed.
If needed, close Control and run Collect-PT-Primary-M12R53.cmd for its results ZIP.
