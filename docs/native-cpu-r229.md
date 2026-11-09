# R229: repair streaming pin allocation

Run **Start-CPU-R229-Pinned.cmd only**. Fully exit the previous game process first. Use the same problem area and a short traversal, then collect compact logs and report texture arrival/stutter. No Reference run, video recording or repeat standalone DirectStorage test is required.

Both phase instrumentation and R226 Resolver remain disabled. The launcher still requests an 8 ms budget and 16 partitions. No visual settings, streaming budget, native request processing or RT caches are changed by this repair.

## R228 result

The user reported instant texture arrival and some traversal stuttering. The latest session in the supplied ZIP (20261030-143326-294-42612) contains `STREAMING_R228_INSTALL ready=0`, followed by successful initial application of 8 ms / 16 partitions. Therefore this run does not verify persistent partition pinning. It also confirms preparation phase hooks were absent.

The generic relay allocator used by R228 searches from near the lower end of the callsite's signed 32-bit reach. The relay also needs to reach the partition global, about 8 MiB above the callsite. A valid CALL allocation can consequently fail the store's range check. R228 failed safely, but did not log the specific reason; this is a confirmed implementation defect and a plausible explanation of the live failure, not a directly logged diagnosis.

R229 allocates within the intersection of both address ranges. It retains the register/flag-preserving leaf store and adds explicit installation failure reasons. The existing five-second CPU report now reads the current budget and partition count without rewriting either value. Expect:

```
STREAMING_R229_INSTALL ready=1
STREAMING_R229_STATE ... pin_installed=1 budget_ms=8.000 slices=16
```

Executable regression fixtures use an 8 MiB callsite-to-global gap and the production intersection allocator. They also cover both native stores, 8/16 overrides, arithmetic flags, R12, LastError, target mismatches and rollback. A boundary case reproduces why the old one-sided range was insufficient.

## Timing limits

The sampled 124.462 ms RT-record maximum and 13.689 ms material-update maximum already appear in the first report with RT calls (present 11340, 169 calls). These maxima never increase during the later run. They cannot establish the cause of the reported traversal stutter. Sparse whole-stage samples do not capture every frame or separate streaming I/O, decode, upload and publication.

Preserve the observed fast texture arrival as the responsiveness goal. A stutter fix still requires evidence; this build repairs pin installation and makes the live configuration observable. DirectStorage remains a standalone validation foundation, with no native game texture redirection yet.
