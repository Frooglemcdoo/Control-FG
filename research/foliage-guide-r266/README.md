# Second captured foliage lighting variant

Source: ControlTOH-Materials-R258-20261101-002336-83c27dc2.zip. ZIP CRC and manifest hashes verified. PID46604/session24923, family4/batch495/frame24923. Original PS12956 bytes SHA2563755ae5f574c639ee79de158a309414dec754dcc4964ccfa477322f7ef1f658a. Original/replay VS and replay PS are byte-identical to R265 evidence.

Native instructions: dword417 computes material index times35; dword427 samples RGB from material texture array slot0/space1 with v1.xy and sampler19/space1; dword440 multiplies diffuse lighting by that albedo and adds specular. No discard. Native replay supplies the same texture RGB with alpha1. The source-audit test pins these instructions and resource bindings.

Runtime selects the evidence candidate by native PS byte size and then retains full byte equality for all shaders. It uses one validation call per draw/policy, preserving the bounded Session budget and cache. No blanket foliage admission.

Important: primary user-reported failing plant captured in session4923 has the ORIGINAL R265-supported shader. This second variant is a separate coverage correction; do not claim it resolves the original speckles. Central Research buffer analysis is in ../rr-plant-r265. Hair diffuse/GI findings are in ../rr-hair-r265. Immediate RR output is captured before downstream native postprocessing; no demonstrated raw-light re-add in the evaluation wrapper.
