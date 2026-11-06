# R362: active RT signal isolation and candidate stabilization

R361 run162719-95000-0 completed eight images without failure, eleven ready character PSOs, skin/cloth/hair replacement bindings, active RR, clamp60. All four phases recorded ZERO calls at broad diffuse sites A/B, zero preserved calls and zero bypasses. Effect mask0x6b enables reflections, diffuse GI, contact shadows, transparent reflections and debris; neither sun-shadow nor AO bits are set. R361's correction was not exercised. No conclusion about the effectiveness of that filtering can be drawn from unchanged images.

The user states the remaining distortion appears with RT ON and is absent with RT OFF. Do not confuse this with older RT-off shadow/volume complaints.

## Work completed

R362 traces the existing active Native Blend GI scope and contact-shadow filter. It adds only bounded atomic capture counters and transient overrides, no GPU readbacks or extra producer. Gameplay choices remain unchanged. This build does not claim a new confirmed rendering fix.

TEST JESSE runs baseline / GI removed / stabilized GI / contact filtering preserved / both / baseline repeat. Character PSO variant10, HQ rig/near history, camera/settings, RT, RR and clamp60 stay constant. Each phase has five seconds settling and two full-client JPEGs. GI removed is a causal diagnostic, never a gameplay default or fix; removing illumination can also hide material-dependent artifacts and does not prove an estimator error.

GI overrides are latched per native filter invocation, use the existing exact shader variants, and never mutate stageMode or viewRequested. GI removed selects ClampOnly plus the Zero view. Stabilization selects the existing motion-adaptive temporal plus steady spatial variants (BothSteadySpatial). These are candidates for this remaining character artifact, not newly invented filters or proof of moving-character history correctness. Existing shader material/depth/history rejection and all bindings remain unchanged. Contact preservation forwards original temporal flag, spatial size/step and resource arguments. If the spotlight path already preserved these, the baseline counter will show it and the phase cannot be treated as a changed contact policy.

The saved RAW BYPASS preference remains displayed while transient diagnostic overrides run; the manifest records the source override separately. Every finish/cancel/focus-loss/timeout path clears request to0. GI thread-local scope restores even if native calls throw.

## Evidence contract

GI entry and eligible-scope counts distinguish native fallback from an admitted scope. Temporal and spatial counts require the scoped InvokeGI dispatch to report substitution. Zero/raw/steady counters are incremented only after the corresponding replacement PSO's actual command-list Dispatch returns. Missing optional shaders or count!=1 fallback cannot claim coverage. These are CPU-recorded commands, not proof of completed GPU execution.

Contact entry/native/neutral counters record call paths after forwarding the native function. Separate contact temporal/spatial counters require a forwarded temporal=true or positive spatial size/step. Candidate coverage requires zero PSO dispatch for GI removal; eligible+temporal+steady dispatch for GI stabilization; contact entry+native call+nonzero temporal/spatial filtering for contact preservation; and both for combined. Missing coverage skips that phase's images and continues the other phases, recording incomplete rather than clean success. Baseline counts reveal whether any candidate actually changes a previously neutralized path.

## Validation

Portable checks exercise every transient phase and saved GI stage/view, original-setting preservation, partial/missing coverage and counter attribution. Windows tests extend the real GI invocation scope test, including restoration, and the existing exact shader WARP tests verify zero/steady dispatch counters, optional steady-PSO fallback and real GPU outputs. The capture remains RR-read-only. The clean-baseline audit reverses exact contact-hook edits before checking all132 baseline hashes. Actual game visual diagnosis remains pending.
