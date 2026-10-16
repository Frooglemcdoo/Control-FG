M12R71 - late-admission history and environment source correction

R70 capture evidence: all 22 reconstruction records had history=0; every logged
producer admission used late_signal=1. History was published only at Submitted,
before late COPY admission had armed the frame. R71 publishes after BOTH events
regardless of order, retains the output, and rejects probes/cancelled/older frames.
Reconstruction logs now include previous_frame for verification.

R70 also logged environment=0 throughout. The replacement sampled a cube captured
from the reflection composite but multiplied it by the opaque-pass provider.
R71 captures b1 byte244 from that same reviewed composite (native dword2962),
validates finite/nonnegative intensity, and rechecks it before producer admission.
No arbitrary brightness floor is introduced. The new log identifies the source;
in-game capture must confirm its actual intensity and missing-light improvement.

A failed primary direct-light evaluation no longer publishes covered black.
A valid primary estimate still survives an unsupported secondary hit.

Close Control and replace all package files beside dxgi.dll. Existing PT selection
and game settings are preserved. Do not rerun PT-Enable if PT is already enabled.
Collector: Collect-PT-Primary-M12R71.cmd.

Tests include both history callback orders and rejection/lifetime cases, GPU
primary/secondary lighting-failure behavior, reconstruction accumulation and
rejection, and root-buffer composition. In-game quality remains unverified.

Limits: native passes still execute; full FPS redesign and all-material/all-light
coverage remain unfinished. Uniform one-light sampling can remain noisy, history
has no object-motion vectors, and unsupported secondary materials can lose energy.
