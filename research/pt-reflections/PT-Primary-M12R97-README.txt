M12R97 - bounded continuation misses use authored GI

Corrects a missing lighting path with RR either ON or OFF. R96 used the
sky/environment cube for every miss; its scale was zero in the supplied capture.
R97 applies the archived native reflection ray-extent formula to secondary
continuations when the authenticated GI resources are available. A missed ray
requests directional baked GI at its finite endpoint. The path throughput is
applied once; this replaces the miss term and does not add cache light at every
hit. The existing fourth-hit diffuse tail remains. Outside valid cache coverage,
the environment cube remains the fallback; valid black GI stays black.

This is a native-style finite-range approximation extended to the PT path,
not an unbiased full-world path tracer or proof of native rendering equivalence.
It can change distant reflection detail, and the cache has static-light and
light-leak limitations. In-game floor lighting/reflections still need comparison.
Secondary light-list coverage, material/normal fallback, texture flicker and RR
reflection-motion inputs remain unresolved; this package does not claim otherwise.

No extra full-resolution buffers or DXR callable GI are introduced. The existing
GI compute pass consumes the miss request. New sparse log records report miss
requests and positive cache samples; those counters are not visual proof.

Close Control and replace package files in the same installation. Keep settings;
do not rerun PT-Enable. Compare the affected floor from the same camera with PT
OFF and ON, then run Collect-PT-Primary-M12R97.cmd and send the results ZIP.
