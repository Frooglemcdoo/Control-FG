M12R78 - reconstruction storage capacity and explicit failure reporting

R77 logs confirm producer_recorded=0 for all sampled consumer rejections,
alongside surface_reconstruction_setup failures. These are missing producers,
not evidence that valid producers executed in the wrong order.

The signal pool had a 2 GiB cap while raw/final output storage permits 4 GiB.
At 4K each signal buffer is 265420800 bytes. Four signal/scratch pairs plus one
retained history signal exceed 2 GiB, despite output capacity allowing that
many frames. Recording ownership and GPU retirement can retain these buffers.
R78 raises the signal cap to 4 GiB, preserving all ownership/fence protections.
This is a ceiling, not a preallocation; peak VRAM can increase under pressure.
It corrects a reproducible capacity mismatch. R77 did not log allocation stages,
so it does not prove that every setup failure was this cap rather than another
resource or input failure.

PT_PRIMARY_RECONSTRUCTION_FAILED reports exact setup stage, HRESULT, resident
and idle signal bytes, budget, and cumulative cap/device allocation failures.
Tests verify 4K retention arithmetic, bounded capacity and non-aliasing while
storage remains referenced, plus existing GPU reconstruction and lifetime tests.
R77 history clipping remains. No settings changes or extra rays.

This does not complete RR hit-distance integration, object motion, missing-light
sources or establish that all visible flicker/noise is fixed. Game validation
remains necessary.

Close Control and replace ALL package files while preserving settings.
Do not rerun PT-Enable. Collect with Collect-PT-Primary-M12R78.cmd.
