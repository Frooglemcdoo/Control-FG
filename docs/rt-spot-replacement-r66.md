# R66: first selected-spotlight RT replacement test

This experimental build connects full-distance spotlight rays to native selected-
light composition. It is not a complete all-light shadow replacement. Visual
quality, caster coverage, softness and GPU cost require an in-game test.

## Test

Close the game to install the DLL and three rt_spot shader files beside
Control_DX12.exe. Preserve NVIDIA runtimes and ControlFGStreamline. Load the
problem room in Borderless with FG, RR and glass enhancements off. Enable native
RT Contact Shadows. F10 opens NATIVE SHADOWS / RT SPOTLIGHTS. Compare from the
same position, then move Jesse and the camera. No restart between modes.
SAVE LOG writes one Lighting-R66 log. Send it with the visual result and FPS.
If RT SPOTLIGHTS is unavailable, save the log; do not disable native shadows.

## Implementation

The captured single-ray DXIL variant is augmented with an additional raygen export
in the same state object. Native hit/miss identifiers and tables remain valid.
A separate immutable 64-byte upload allocation holds its 32-byte identifier and
is retained by the native state object. Native alpha testing is retained.

Only spotlight TMax, initial payload and hit-distance comparison change. TMax
uses the native receiver-to-light distance minus near distance, clamped at zero.
The existing angular sampling radius retains the original contact-distance
calculation. Point rays remain native. This preserves the native sampling
calculation, not a promise that full-distance penumbrae match the old map filter.

The selected-light compute variant sets the final spotlight map visibility to
one BEFORE the map's zero-visibility gate. Its existing filtered RT visibility
then weights the selected spotlight's diffuse/specular contributions. Other
lights, intensity, color and falloff stay on native paths.

The reviewed renderer call at RVA 0x12daf7 to 0x23ed10 encloses ray production,
filtering and selected-light composition. A thread-local scope holds the live
selection across that call. The composition alternate requires an observed
full-distance ray dispatch in that same scope. Mode changes select a current-
visibility-only temporal shader for one pass; subsequent passes use native
history. Native spatial filters remain active.

Exact SHA256 checks protect shader matching and sidecar loading. Unknown shader
variants retain native behavior. This first runtime supports the single-ray
library captured in R65; the configurable multi-ray library seen in R64 stays
native. The generator supports both for further integration. If full-distance
rays are recorded without composition, the experiment disables itself for the
session and logs the mismatch. A first failed pass may still have used the
longer rays with native map composition.

## Validation and limits

DXC's assembler and validator generated and signed the replacement DXIL.
Windows CI validates the two modified compute shaders by disassembly and WARP
pipeline creation, compiles the proxy, and runs baseline/menu/FG tests. These
checks do not establish DXR state-object linking on the target GPU, GPU execution,
visual quality, or complete scene coverage. Command recording is logged separately.

The renderer chooses one spotlight per pixel based on native light contribution.
Unselected lights retain native shadow maps. Native geometry inclusion masks and
shadow-caster culling may limit long rays; contact-shadow scene coverage is not
assumed to cover every distant caster. Native denoising may need further changes
for full-distance rays and changing selected-light identities. Single-ray sampling
can still be noisy. This build does not claim to remove all speckling.

Build inputs under assets/rt-spot are generated text-encoded shader containers
with hashes in manifest.json. tools/generate-rt-spot-pair.py reproduces them from
hash-verified captures with DXC, tools/assemble-dxil.cpp and DXV. It retains native
resource layouts and limits instruction changes to the reviewed anchors.
