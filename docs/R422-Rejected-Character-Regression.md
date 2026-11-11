# R422 rejected: regression in moving character lighting

User feedback: "whatever you did made it much worst". Treat R422 as failed, not an improvement requiring stronger settings.

Capture: GI-Motion-R422-5476-329837156.zip.
SHA256: 2218ead51b89caabdc7c7da2284ec9225bf0d247647fc25fda8b0a1b80b5e749.
Frames 9943/9944 are consecutive; RR result 1, reset 0, stride one, 2560x1440 input and 3840x2160 output. Pre-exposure remains 5.55555534. Pre-GI/RR-bound GI RGB differences are zero in both frame counters.

Inspection shows broad mottling on Jesse's neck and red patches in her hair. The structure is already visible in the input color and survives in RR output. The diffuse guide retains strand and skin detail. Inspected cropped buffers are finite. This is a different view from the R421 captures, so cross-build pixel statistics would not be a controlled A/B.

R422 removed the radiance-dependent neighbor veto. Spreading bright samples into spatially correlated patches is consistent with the observed failure, but the capture does not by itself prove every patch's exact source path. Do not claim a newly established wrong-surface remap or defective motion vector.

The approximate offline replay measured reduced local residual RMS with near-preserved ROI mean, not successful RR reconstruction. It lacked native reconstruction maps and RR execution. That evidence was insufficient to predict gameplay improvement. Passing shader/GPU tests validated code behavior, not visual acceptability.

Disposition:
- Reject R422 and do not strengthen its spatial averaging.
- Existing verified R421 is the pre-regression fallback; it retains the original unresolved artifacts.
- Before the next rendering candidate, restore R421's radiance rejection and preserve this failure record.
- Do not repeat stationary tests, request another generic capture, or treat stronger RELAX/clamping as the next fix.
- The unresolved question remains sample/surface correspondence versus valid high-variance lighting. The current captures omit the native packed reconstruction map and per-fragment coverage. Do not label that missing evidence a proven fault.

