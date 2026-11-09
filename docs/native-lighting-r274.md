# R274 — bounded GI spotlight arithmetic capture

R273 identified native GI variant 6 during the submitted gameplay run. All eight
variants shared root SHA256 `451162124dcb4e610eb1ee4939b9f0d7963ec3be5d475ed8986799846a22bee3`.
R274 instruments only that observed variant and requires that exact root identity.
Other shaders and normal launches remain native. This is a diagnostic, not a fix.

## Run

Extract the package beside Control_DX12.exe. Run
`ControlTOH\Diagnostics\Start-GI-Spot-Trace.cmd` with Control closed.
Use RR ON, RT indirect ON, Native Blend, FG OFF and GI RAW BYPASS.
Keep the desk lamp intact. Frame the original plant and Jesse's hair, close the
overlay, hold **F7 for one second**, release, and stay still for 10 seconds.
Exit normally. The launcher collects the trace and logs automatically.
Do not use CAPTURE RR or PACKAGE RR. Only one trace is allowed per process.

## What is recorded

One native GI dispatch; one pixel per 32x32 cell across the image; every executed
spotlight iteration at those pixels up to 65,536 records. A 16-byte header contains
the attempted record count; records are 256 bytes. Overflow is explicit and means
the retained records are an incomplete scheduling-dependent subset.

Each record contains pixel/ray/light IDs, raw coefficients at bytes 68/72/76 of
the light record, RGB/range, distance squared, denominator before its native
floor, attenuation, range times attenuation, combined visibility before early-out,
conditional cookie color, diffuse factor and gated RGB before the spotlight MAD,
secondary albedo, primary throughput, secondary position and normal. Valid-stage
bits distinguish unexecuted branches from measured zeros. Vector padding is not
interpreted as an additional material or lighting field.

This does not yet measure final per-pixel depth fade/ray normalization, capture
all pixels, prove every speckle is sampled, or identify a faulty light formula.
The isolated spotlight term is `factor * gatedRGB`; secondary albedo and primary
throughput are separate later factors. It is not the accumulated final GI pixel.

## Runtime safeguards and validation

The instrumented shader retains every original instruction, operand and order;
inverting only inserted instructions and the temp-count change recovers the
native DXBC container exactly. It adds a private raw UAV at u0/space2 via an
appended root parameter. Existing descriptor heaps are not copied or replaced.
All observed native root arguments and the original PSO are explicitly restored.
All seven arguments used by GI variant 6 are required. Its unused space-0 sampler
table may remain unset; the probe does not invent a binding for it.
Capture is rejected if complete root bindings or the exact identities are absent.

The buffer is bounded at 16 MiB plus 16 bytes. Initialization is done during
pipeline preparation. The recording list and generation are retained; readback
is exported only after a fence signaled by its submitting queue completes.
No CPU wait or disk write occurs in the render dispatch. Failure/timeout retains
resources and does not rearm them. This avoids the multi-gigabyte RR image path.

Windows validation creates the instrumented native PSO with the observed root
layout, then runs the exact inserted bytecode blocks with deterministic register
values on WARP. Checks cover record identity, stage masks, a fully shadowed
sample, a skipped cookie, multiple rays/lights, overflow guards and debug-layer
errors. This validates recording mechanics, not live light operand values.

References: Microsoft [Using a root signature](https://learn.microsoft.com/en-us/windows/win32/direct3d12/using-a-root-signature)
and [Root signatures overview](https://learn.microsoft.com/en-us/windows/win32/direct3d12/root-signatures-overview).
