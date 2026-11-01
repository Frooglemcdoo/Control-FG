# GI spotlight trace implementation checkpoint

The recovered R271 images and hit arrays remain the input evidence. A repeat
of that capture is not requested. R272 fixes capture recovery, not lighting.

`tools/audit-gi-spot-trace-sites.py` now resolves the secondary-hit spotlight
instrumentation sites independently for all eight pinned GI variants. The
generated contract is `research/gi-source-r267/spot-trace-sites.json`. It records
variant-local register operands and exact instruction positions for coefficients,
light RGB/range, visibility, distance squared, denominator, attenuation, cookie,
and diffuse accumulation. Missing and duplicated coefficient loads are rejected
by negative tests against each actual decoded variant.

The visibility result must be captured before its zero-visibility early exit.
The cookie sample is conditional. Unexecuted stages need validity bits, not
zero-filled fields presented as observations. At the spotlight MAD, capture its
inputs **before** the instruction; the accumulator afterward also contains
previous light contributions. Secondary albedo, primary throughput, and final
ray-count/depth scaling are later operations: that MAD is not the final pixel
radiance and the contract explicitly records these limitations.

Runtime integration is not enabled and no new diagnostic build is released by
this checkpoint. Required work before asking for a run:

1. Validate the actual GI root layout and mirror its descriptor bindings. The
   existing primary-spotlight capture's 49-descriptor layout is not evidence of
   the GI layout; GI also uses an unbounded material texture array in space 1.
2. Bind a private bounded FP32 trace buffer without changing native outputs,
   with explicit valid-stage, overflow, frame, batch, ray and light identifiers.
3. Preserve the native arithmetic and capture the later material/throughput
   factors so the spotlight term can be reconciled with the final GI value.
4. Verify exact descriptor/PSO restoration and command-list/queue provenance;
   export only after the owning submission fence completes.
5. Test a synthetic light with known attenuation, a fully shadowed light, a
   disabled cookie branch, multiple lights and buffer overflow. GPU readback
   must match the expected record fields before an in-game capture is requested.

Do not reuse the primary spotlight diagnostic as evidence about GI's secondary
shadow lookup. Do not label this static contract as a completed runtime trace.
