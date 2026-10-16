M12R72 - weighted light candidates and measured history acceptance

R71 capture confirms consecutive history is supplied after late admission.
It does not measure pixel acceptance. Environment intensity remains zero even
from the corrected bound reflection-composite source; no brightness floor is added.

Replacement PT now resamples four independent light candidates using incident
luminance and the surface cosine, then traces visibility only for the selected
candidate. Zero-contribution candidates remain in normalization. This is local
RIS, not full ReSTIR or temporal/spatial reservoir reuse. Legacy additive PT keeps
its prior single-candidate implementation. Shadow-ray count is unchanged; light
record evaluation increases, so FPS improvement is not promised.

Sampled GPU counters report valid/current history pixels, projection, previous
coverage, compatible material/normal, accepted history and accumulated sample sum.
This distinguishes supplied history from actual GPU reuse. No threshold is loosened.

The session marker now matches M12R72. The collector preserves failure evidence.
Close Control and replace all package files. Keep current PT/game settings.
Do not rerun PT-Enable when already enabled. Run Collect-PT-Primary-M12R72.cmd
for the next result bundle.

Validation covers shared RIS normalization, zero/invalid proposals, sparse-light
mean/variance, occluded-light energy, GPU RIS readback, and history acceptance
counters. In-game noise and lighting remain unverified. Native-pass removal,
area/emissive-light sampling and all-material support are still unfinished.
