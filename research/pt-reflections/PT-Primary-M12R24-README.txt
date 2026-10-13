Control-FG Primary PT M12R24 - canonical fence identity and surface view matching

R23 results contained deferred dispatches but zero armed/traced GPU samples. R24 matches releasing signals by the capture catalog fence ID, which is canonicalized through IUnknown. Pending-wait diagnostics also retain the canonical identity instead of an ID3D12Fence interface address. The regression fixture deliberately gives those interfaces different addresses.

Surface draws match their bound camera fields against a retained producer camera from the same engine frame. This avoids repeated provider/TLS camera reconstruction for unrelated draws and prevents worker TLS from substituting another view. Exact bound-camera and submission checks remain required. Native fallback stays in place when there is no match.

Replace all package files. Enable PT Reflections through F10 for 10 seconds, report off/on FPS, then run Collect-PT-Primary-M12R24.cmd and send the ZIP. Windows/WARP validation does not establish in-game activation, image quality or FPS recovery.
