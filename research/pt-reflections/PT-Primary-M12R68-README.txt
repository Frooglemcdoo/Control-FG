M12R68 - repair unstable surface routing introduced in R67

R67's video shows whole material patches changing lighting. The collected log
has one readiness transition, no admission rejection, and sustained armed PT
coverage. This does not support the earlier whole-frame readiness failure.

Two routing defects were found in source:
1. Only 64 private surface tables were reserved per frame. Draw 65 onward
   silently kept native lighting; parallel worker order could change which
   objects received PT. R68 reserves 1024 bounded, unique tables and reports
   usage. Exhaustion or reservation failure latches native fallback until
   restart, rather than silently selecting a moving subset each frame.
2. Draws could only bind PT after the producer was recorded on the CPU. R68
   reserves the zero-initialized output and tables at the earlier opaque DXR
   job, retaining the frame in its scene candidate. Draws may record first;
   submission still requires the producer to precede them on the GPU. Missing
   or rejected producers leave native fallback. Resource retirement remains
   tied to submitted producer and consumer fences.

Validation: randomized 200-draw scheduling across 40 frames; unique slots to
capacity, no wrap, short reservation rejection; Windows WARP consumer-recorded-
before-producer test, admission/replay lifecycle, and both host builds. These
verify the repaired contracts, not a guarantee that every source of image
noise is resolved. The transport shaders are unchanged from R67.

Close Control and extract all files beside the existing dxgi.dll. Your PT
startup selection and game settings remain unchanged. Do not rerun PT-Enable
if PT is already enabled. Check the same entrance view from the video first.
If reporting results, close Control and run Collect-PT-Primary-M12R68.cmd.

This remains the surface-lighting integration branch. Unsupported materials
keep native fallback; native passes and their costs remain. Full light/material
coverage, soft area shadows and the CPU/queue performance redesign are not
completed by this routing fix. No FPS gain is promised.
