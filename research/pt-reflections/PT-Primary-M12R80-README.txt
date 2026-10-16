M12R80 - PT diffuse albedo guide for Ray Reconstruction

R79 capture confirms fresh_rr_input=1, history=0 and successful RR evaluations.
No reconstruction setup failures were logged. Two early consumer rejections
had no recorded producer; this build does not claim those cases are resolved.
The user reports improvement, with residual noise/instability visible in video.

The native reference diffuse guide was derived from the packed specular material
value / GBuffer specular channel. PT instead uses textured color * (1-F0) as kd.
R80 supplies that actual PT diffuse coefficient to RR for covered, matching
material pixels. Native normal and specular guides are retained. Uncovered,
unsupported, nonfinite or mismatched pixels keep the native diffuse guide.
Black diffuse reflectance remains black: the 0.02 demodulation floor is applied
only inside the standalone filter, not stored as the material's albedo.

The existing RR guide pass reads the exact same-frame PT raw/surface buffers,
only after their producer was submitted and armed on the same queue. Source
ownership stays in the RR slot until its retirement fence. No wait for the PT
recording mutex: unavailable sources fall back with an explicit reason.
RR_PT_DIFFUSE_GUIDE logs binding status, engine frame, extent and rejection reason.
No added rays or dispatches. RR-off and PT-off paths retain their behavior.

Validation includes the actual RR guide shader on WARP: colored and black PT
albedo, uncovered/raw-invalid/material-mismatch/nonfinite/out-of-range cases,
unchanged normal/specular guides, and PT-off -> on -> off transitions. Existing
reconstruction/lifetime/settings checks remain. In-game quality is not proven
by WARP; specular hit-distance/motion and missing lighting remain separate work.

Close Control and replace ALL package files while preserving settings.
Do not rerun PT-Enable. Collect with Collect-PT-Primary-M12R80.cmd.
