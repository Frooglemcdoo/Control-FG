# R308 — Live native/new GI sampler comparison

The R307 run (PID 58340) validated all three supplied files by size and SHA256.
It logged creation of experimental variant 0 and transitions to raw_bypass.
The user reports that GI RAW completely removes the plant/hair issue. This is
visual feedback, not an image measurement or proof that every scene is fixed.

R308 adds **GI SAMPLER: NATIVE / NEW** to the F10 lighting panel. Both raygens
are exported in the same DXR state object. The native export/library is original
Control code; NEW uses the R307 initial sampling shader. Hit/miss tables, native
material and alpha handling, ray counts, RR and filter settings stay unchanged.
The replacement raygen record is immutable and owned by the state object.
Creation or record-preparation failure leaves the original native pipeline.
Unknown libraries and raygen records carrying local arguments are not replaced.

Install beside Control_DX12.exe preserving folders. Run
ControlTOH/Diagnostics/Start-GI-Rendering-Test.cmd, choose 1, and retain Native
Blend/RR. Select GI RAW BYPASS. In F10, click GI SAMPLER to compare NATIVE and NEW
in the same scene. Close F10 and allow the image to settle after each switch;
RR history is not forcibly discarded. WAIT means no supported pipeline is ready.
Selection applies to subsequently recorded GI dispatches; it is not persistent.
Exit normally for logs. No capture hotkey is needed.

This remains initial GI sampling, not full ReSTIR, higher resolution, a new
lighting model or sample reuse. Native here means original GI sampling under
the same current filtering/RR settings, not all-vanilla game rendering.

Validation includes both source hashes, descriptor/association rebasing, native
library preservation, renamed export, local-root rejection, failed creation
fallback, toggle round trip, and the existing R307 arithmetic/DXIL checks.
Runtime DXR export creation and visual comparison still require the game run.

## Delivered build

Foundation run 41684487395 and full Windows build 41684487615 passed. The full
build includes the toggle fixture, WARP lighting tests, DLL compilation and menu
checks. Artifact 13255192178 was downloaded and verified: outer/inner CRCs,
outer published digest, inner packaged checksum, DLL R308 marker and both button
labels, and updated launcher. There are 75 install entries, 4,276,421 bytes.
Install ZIP SHA256:
`23908f00375528aa520edcec3a02b31a5ec2b21f84fc958688ed10c25a278385`

DXR export/identifier basis:
https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html
The added raygen is explicitly renamed; its identifier comes from the same
state object used by native tables. Validation is not hardware/game execution
proof. Logs distinguish requests from subsequently recorded dispatches.
