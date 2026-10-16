M12R77 - current-signal history clipping and producer fallback diagnosis

R76 has 34 sampled consumer rejection records, all reason=1. Queue, view,
descriptor and replay checks passed. This does not distinguish an absent,
unarmed, deferred or cancelled producer and is not proof of incorrect GPU order.
No producer admission or late-admission rejection was logged. The old global
skip throttle could suppress rare producer failures behind surface-binding skips.
RR evaluations reported success while RR was enabled; the capture also includes
explicit user RR off/on toggles. PT signal reconstruction remains our own filter.

R77 clips diffuse and specular temporal histories separately against compatible
current-frame demodulated samples (3x3, three-sigma bounds intersected with sample
range). Second moments follow the corrected mean. Uniform lighting-off changes
no longer retain old lighting merely because geometry is unchanged. Moving
specular accumulation is shortened because surface motion is not reflected-hit
motion. No rays, lighting gain or settings changes. Clipping is a biased stability
filter and can reduce rare contributions; this is not a complete denoiser.

Skip logging now counts each reason independently. Consumer rejection records
are sampled by distinct frame and include producer recorded/submitted, armed,
deferred/cancelled and batch position. GPU ordering safety checks remain enabled.
Native-light fallback is NOT claimed fixed. Object motion and PT-specific RR
hit-distance integration remain unfinished; noise/smearing may still occur.

GPU regression checks both legacy and split paths, noise reduction, energy,
material/depth/cut rejection, and immediate removal of stale split lighting when
the current compatible surface is dark. Target-game visual validation pending.

Close Control and replace ALL package files, preserving existing settings.
Do not rerun PT-Enable. Use Collect-PT-Primary-M12R77.cmd for results.
