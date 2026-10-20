# R51: isolate native lighting output dither

Status: diagnostic build; visual benefit requires an in-game comparison.
Baseline: R49 (RTX 40 provider fix and native graphics menu). R50's changes to
native filtering are not included, so this is a single-variable experiment.

## Evidence

All 509 exported shader files passed their supplied SHA-256 checks. Native HIGH
and LOW each contain the same 109 unique shader binaries. The DLSS/RT-OFF
capture also contains 109, with six binaries different from the native set.
Shader identity alone does not prove identical constants, resources or outputs.

The executed HIGH compute PSO 3638, also present in LOW and DLSS/RT-OFF, is the
native deferred lighting shader, 69,704 bytes, SHA-256:
`bee42ac080862114b7a5926c3053c78162d70de339b20a954b929ca231ccd2ff`.
The HIGH command list binds this pipeline at source line 10497.

Its end-of-shader sequence samples `g_tBlueNoiseOutputDither` at t4, subtracts
0.5, computes `1 + noise * (0.02, 0.10)`, and multiplies the diffuse/specular
outputs immediately before their UAV writes. For noise in [0,1], these are
up to +/-1% diffuse and +/-5% specular variation. Coordinates depend on
`g_uTemporalFrame & 7` with offsets (59,61) and a 1/128 scale.

The shader separately samples `g_tBlueNoiseShadowJitter` at t5. Output dither,
shadow sampling jitter, screen-space effects and film grain are distinct.
This capture does not establish which contributes most to the visible noise.
`g_uRTContactShadowRayCount` is listed in reflection metadata but marked unused
in this non-RT shader; changing it is not a supported remedy for these captures.

## Experiment

An adjacent `ControlFG-Native-Dither-Off.txt` marker enables the test at startup.
Only the exact SHA-256 and exact full MAD instruction are admitted. Two immediate
amplitudes become zero; the original instructions, sampling, resource bindings,
root signature, thread layout and shader length remain intact. The checksum is
replaced with the precomputed checksum for that exact patch. A second SHA-256
check must match `f451686b9d84e3fadfa95faab1d276115c08469cf0d11b601432e3eb7caa19c7`.
The cached native PSO blob is cleared for replacement creation. Driver rejection
falls back to the unmodified native descriptor. Logs report applied/failed.
No additional GPU pass, texture or sample count is introduced.

This targets the captured non-RT variant only. Do not infer RT/RR coverage.
The test does not change the skin guides or glass shaders. The native lighting
outputs it modifies can affect any materials consuming those outputs.

## Validation and test

Local C++ contract check against the captured shader passed. Output equals the
independently generated patch byte-for-byte. Decoding both versions showed one
instruction difference: (0.02,0.10) -> (0,0). Resource bindings and constants were
identical. All 509 native DXBC checksums validated with an independent checker;
the modified container checksum also validated. The existing 132-file baseline
audit passed with exact, reversible hook exceptions. Windows build validation is
separate; Linux cannot verify the actual driver or visual result.

Install the package with Control closed. First test RT/DLSS/RR/FG/glass OFF at
the same pose as capture 01. Let the view settle. Compare the included marker
against renaming it to .disabled, restarting Control between both cases.
Confirm `NATIVE_LIGHT_DITHER_R51 matched=1 applied=1` in the enabled run.
Check speckles, gradients/banding, motion and FPS. Dither removal may reveal
quantization banding. If the pattern remains, this test isolates only one source;
shadow jitter and screen-space filtering remain candidates.
