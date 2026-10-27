# R190: compact specular hit distance for RR

## Motivation and limits

R189 was enabled and created successfully in the user's SPEC4/RR-on run, but
the user saw no improvement. Its roughness blend is disabled in R190. Rendering
returns to the accepted R184 hit-view / R176 retry-reuse baseline.

The prior captures had no specular hit-distance input and cleared reflection
motion vectors. NVIDIA describes hit distance plus matrices as an alternative
to directly supplied specular motion vectors:
https://github.com/NVIDIA-RTX/Streamline/blob/main/docs/ProgrammingGuideDLSS_RR.md

That documentation emphasizes reflection quality during motion. It does not
establish that the missing input causes this user's stationary splatter. R190
is a controlled integration experiment, not a claimed stationary-noise fix.

The existing D1 implementation was disabled by default because its three-slot
material/position copies retained every ray layer. Its distance shader reads
only ray zero. R190 keeps that shader and its authenticated producer constants,
frame/queue handoff and fence retirement, while copying only the data it reads.

## Test

Close Control, install the inner ZIP beside Control_DX12.exe and preserve the
NVIDIA runtimes. Run **Start-Compact-Distance-Test.cmd**.

| Option | Behavior | F10 label |
|---|---|---|
| 1 | Request compact hit distance for RR | HIT DIST: REQUESTED |
| 2 | Baseline, no hit-distance capture hook | HIT DIST: BASELINE |

Use NATIVE BLEND, RR ON, FG OFF and SPEC4 every launch. Let the scene settle,
check the painting and elevator with the camera stationary, and SAVE LOG for
each option. Compare FPS as well as splatter; note any new reflection trails
when moving. Both options retain full lighting at normal brightness. The
failed R189 filter and earlier 8x isolation modes are disabled.

The label describes the request, not success. The Lighting log's
rr_compact_distance record shows requested, installed, evaluations,
bound_evaluations, last_bound, status, capture_status and capture_bytes.
Nonzero bound_evaluations means the hit-distance parameter was supplied and
read back successfully for those evaluations; it does not prove GPU completion
or image-quality improvement. Rejections remain unbound, with status evidence.

Normal launch requests this experiment. Option 2 disables it only for that
launch. No settings file is modified. No full capture is required initially.

## Data and cost

Private material and position textures have one array layer. Native source
arrays, sample counts and ray-generation shaders remain unchanged. The copy
uses subresource zero on both source and destination, with explicit matching
format/dimensions and single-mip/sample validation. The unchanged distance
shader receives one-layer SRVs and a matching private capacity constant.
Native source constants are validated before this capacity adjustment.

Distance is derived from the primary surface and the first secondary hit in
world units using the existing authenticated matrices. It is not primary
depth. The existing miss/no-ray sentinel and status classification remain.
Ray zero is a representative sample, not a radiance-weighted distance of all
SPEC4 hits. Different hits across rays and frames remain a limitation.

Copy payload and material/position allocation no longer scale with native
array depth: each private slot is approximately 10 bytes per input pixel.
The three slots total approximately 105.5 MiB at 2560x1440, or 237.3 MiB at
3840x2160, plus alignment. The separate distance/status outputs add about
84.4 / 189.8 MiB respectively. These are estimates, not measured in-game VRAM.
The log reports actual capture allocation bytes separately from outputs.

There are two one-layer GPU copies plus a distance compute dispatch when the
path succeeds. No new reflection rays, readbacks or CPU/GPU waits are added.
This is additional work versus leaving hit distance unbound; it is not a free
input or a claimed performance gain. Existing three-slot fences, resize
retirement, source retention and fail-closed behavior are preserved.

## Validation

The original and compact backend variants run the existing 165 scenarios,
including allocation/command/hold/signal failures, memory admission and 3000
captured-frame retirements. Compact tests require one-layer allocation shapes.
A WARP test calls the actual production CopyFirstRay adapter for both formats
and native depths 1/4/16, checks every returned byte against layer zero, poisons
later layers, and verifies alias/stale-frame/multilayer-destination rejection.
The unchanged distance shader is compiled with its existing ABI checks.
Existing producer-input, distance/origin math and resize tests remain relevant.
Windows launcher and policy tests cover ON/OFF, invalid values and cancellation.
The complete DLL build is a separate gate. In-game hookup, image quality and
performance still require the supplied logs and visual comparison.

Experimental branch only. No production branch or release changes.
