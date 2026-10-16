# R79 RR input audit

NVIDIA source (accessed 2026-10-16):
https://github.com/NVIDIA-RTX/Streamline/blob/main/plugins/streamline-agent-skills/skills/integrate-dlss-rr/references/integration-checklist.md
Sections “Accumulated Renderer and RR History” and “Path-Traced, Stochastic, and Temporal Inputs” require fresh noisy current-frame radiance and coherent guides, bypassing renderer accumulation while preserving RR temporal history during normal motion.

The implementation violated this: pt_surface_reconstruct.hlsl accumulated split radiance across frames and applied two spatial passes, then the surface replacement shaders fed this result into native scene color. RR evaluation used current native guides. This is a concrete input-contract mismatch; causality for all reported noise/smear is not established.

R79 keeps current diffuse+specular radiance exactly on confirmed full-RR frames. Frame selection is published after native feature result and jitter-option confirmation; a single atomic engine-frame identity prevents UI-request or stale-frame routing. Frame-zero/unknown, partial, warmup, paused, stopped and SR routes do not select fresh input. Each pass captures the decision once. Later fatal RR failures retain existing behavior rather than switching denoisers after lighting has been recorded. The standalone filter cannot reuse fresh-route signal history when RR switches off.

No additional RR reset is introduced for ordinary motion. The coordinator already resets when switching SR/RR, changing effects or rebuilding resolution. Coverage/material/depth gates remain. Fresh output retains current surface guides and count one; custom signal history is marked incompatible. Pool allocation and lifetime policy remain unchanged in this correctness change.

Validation: existing legacy/split WARP suites plus fresh-input sequence that verifies unchanged checker samples, diffuse+specular sum, light-history rejection, zero-coverage and rejected producer admission, and return to standalone filtering without consuming incompatible history. Atomic route tests verify exact-frame selection and clearing. Windows CI compiles strict HLSL and runtime. In-game visual validation remains required.

Limits: RR already receives native primary motion vectors, but moving-object correctness has not been proven. Our standalone filter remains camera-only. PT hitDistance currently records the first mixed-BSDF continuation, so binding it as specular hit distance would mislabel data. No such binding is added. Native passes and unsupported materials remain; environment lighting is absent in the R78 capture. This audit does not establish a CPU bottleneck or solve missing lights.
