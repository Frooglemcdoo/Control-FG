# R211 — GI pipeline replacement without engine-record cloning

R210 stalls during the first GI replacement at frame 6622. The paired probe log confirms RR feature creation and specular filtering completed, then the render thread stopped while the settings thread logged for another seven seconds. Neither supplied log contains an exception stack or device-removal report. The exact faulting instruction is not established.

R211 removes the entire R210 GI shader-record clone and synthetic shader-storage branch. The native temporal bind receives the original record unchanged. No engine object, internal pointer, synchronization object or cached shader descriptor is copied or edited.

The existing compute creation observer (also used for successful pipeline-library loads) recognizes the original 5076-byte shader by full byte comparison. It creates a separate D3D12 PSO with the native root signature, flags and node mask, clearing CachedPSO. Both native and adaptive PSOs are held in a bounded process-lifetime cache. Creation failure or cache exhaustion preserves native filtering and RR.

The validated GI temporal callsite scopes the existing specular-clamp SetPipelineState/Dispatch hooks. Only the observed exact native PSO on the validated current command list/frame may select the adaptive PSO. It is selected after DeviceUtil commits bindings, immediately around the single GPU Dispatch, and the original PSO is restored in a finally block. Missing observations, mismatched contexts or unavailable variants use native dispatch. The GI scope does not modify specular-clamp status or CameraCut.

The adaptive shader is unchanged from R210: native 70% stable history; motion scale 400 instead of 40; relative depth tolerance 0.01 instead of 0.03. Current-frame energy clamp and spatial passes remain native. Image-quality thresholds remain experimental.

Validation includes exact shader-token/chunk audit, actual native and adaptive WARP pipeline creation, the production creation observer and dispatch hook scope with real WARP PSO objects plus a recording dispatch callback, fallback and exception restoration, original-record forwarding, context/frame gates and the existing full build. The recording callback test does not execute the shader over scene textures and cannot establish in-game image quality or launch success.

Test level loading first. Then Native Blend, RR ON, FG OFF: stationary dark corner, Jesse moving over carpet, camera pan. Save F10 lighting and probe logs. Probe reports GI_PIPELINE_R211 ready=1; lighting reports gi_temporal_adaptive build=R211 applied=1, pipeline_ready greater than zero, and continued RR successes. applied=0 is explicit native fallback, not proof of a working adaptive fix.

Accepted R205 projection rays, MAP GUARD OFF, angled reflections and other rendering defaults remain unchanged. Production branch untouched.
