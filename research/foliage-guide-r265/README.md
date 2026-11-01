# Verified plant replay and hair audit

Evidence: ControlTOH-Materials-R258-20261031-144200-fe6b500d.zip, family 4,
batch 95, frame 17430, PID 29768. This is shader-family evidence, not a permanent
material ID or object-specific patch. Native and candidate vertex bytes match.
The earlier differing-vertex foliage variant remains rejected.

Native vertex SHA256: a954d6e56122c5c0008775c37371d1bf240c6349c89c3397c455df655cdc1ea1.
Native PS SHA256: 68b095104f50891b21ebd4ff28c3ec038f1d6062c84f4f4568929a5037bc16a7.
Replay PS SHA256: cd3f9865a91912865a87dbae62a8e41f8a8a7e9281e6f123e90106e6aae73b0b.

Native PS SHEX dword 174 computes material ID * 35; 1312 samples base-color RGB
from material texture array slot 0 using v1.xy and sampler 19/space1. Dword 1325
composes diffuse-light RGB * that base color + specular-light RGB. The replay
computes the same index at 31, samples the same RGB/UV/sampler at 41, writes RGB
at 54 and alpha 1 at 59. It does not output lit color. Reflection declarations
match the native material-array/sampler layout. No native or replay PS discard
instruction is present in this pair. Native vertex equality, existing depth and
draw-range controls remain mandatory; this is not blanket foliage admission.

The captured R257 leaf pixels previously fell back to RGB 0.054901123 because
replay coverage was excluded. R265 corrects eligibility for this verified pair;
per-pixel coverage and visual improvement require the test run.

Hair: user reports R264 speckles mainly when particular other lights hit Jesse.
The submitted 16-second video is visual evidence, not raw lighting/guide data.
Current hair guides already have tagged texture replay, local normal processing,
opacity blending and a negative Model-F responsivity mask. The specular guide
uses the material F0/EnvBRDF path, including the reference BRDF-mode exclusion.
These facts do not identify which term causes the light-dependent artifact.
Do not modify hair masks, normal smoothing, energy clamps or mode exclusions
without a matched lit-hair input/output capture. R265 preserves these paths.

User confirmed R264 removed the projector-floor speckles. Preserve its sampling
change while evaluating the separate foliage correction.
