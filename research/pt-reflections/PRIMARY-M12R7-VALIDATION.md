# M12R7 active pipeline evidence

R6 user log: 3,682 accepted, scheduled and completed native writer jobs;
1,027 ON-mode opaque recognitions and environment dispatches; zero primary
compute recognitions; healthy binding tracker; no missing PSO snapshots.
The reviewed primary PSO was created. No primary render was submitted.
This isolates the current earliest blocker to primary pipeline recognition.
It does not establish a shader variant, proxy identity mismatch, or alternate
command path as the cause. The R6 interface fix alone was insufficient.

M12R7 records SHA-256 and canonical identity for at most 1,024 created compute
PSOs, and separately marks their first observed ON-mode binding and direct
dispatch. This also identifies a bound PSO missing the direct-dispatch path. It retains at most
12 MiB of bytecode for compute shaders whose RDEF contains both GBuffer inputs
and deferredlight_constants. Bound candidates emit chunked bytecode with
size/hash delimiters; unexecuted candidates remain bounded in memory.
Each exported candidate's retained bytes are released after export.

Candidate selection is capture-only. It cannot add a PSO to primaryPipelines
or change roots, descriptors, shader selection or rendering admission.
Unknown executed identities are explicitly logged. Native forwarding remains
unchanged. First-use export can cause a short CPU logging hitch.

The portable selector test checks the recovered primary shader locally,
compute-stage and RDEF confinement, truncated containers and invalid offsets.
The actual game shader is not committed to the repository. The standard Windows
build and WARP tests remain required. Collector results identify executed
lighting candidates and export counts without labeling either as PT success.

The run also exhausted the 512 MiB active geometry-copy budget (6,603 cumulative
failures). That is a separate remaining issue; no larger memory allocation is
introduced by this diagnostic change. In-game capture is required before
changing primary shader admission.
