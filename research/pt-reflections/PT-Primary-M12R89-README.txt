M12R89 - terminal GI compute integration

R88 failed in CreateStateObject: device reason S_OK immediately before creation,
DXGI_ERROR_DRIVER_INTERNAL_ERROR afterward, with no private PT dispatch submitted.
The separate callable shader did not resolve the failure.

R89 retains terminal GI but removes its traversal, brick textures and callable
from the DXR library. Rays emit terminal position, normal, throughput*kd and
primary lobe fractions. A compute pass applies the same GI calculation to the
raw diffuse/specular estimates before reconstruction and RR.

The requests reuse the two existing reconstruction buffers before filtering.
No additional image-sized allocation; all positions/weights remain float32.
The pass runs at actual render resolution. It adds a compute dispatch and memory
traffic; no performance gain is claimed. Existing lighting/reflection limitations
remain under investigation. Native RT GI is not added back to the result.

Validation includes production shader compilation and an explicit check that no
GI traversal/callable remains in DXR; GPU WARP checks of the production compute
handoff, lobe weights, admission and stale-data guards; existing regression gates.
NVIDIA driver creation and in-game results still require confirmation.

Close Control and replace ALL package files, including pt_terminal_gi_compute.dxil.
Keep existing settings. Do not rerun PT-Enable.
After gameplay run Collect-PT-Primary-M12R89.cmd.
