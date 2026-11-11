# Verified package R422

**REJECTED after gameplay testing:** user reports substantially worse artifacts. See R422-Rejected-Character-Regression.md. Build/test success below is historical and does not establish visual quality.

R422 changes character diffuse reconstruction to use surface weights without radiance rejection. It is a candidate visual correction; gameplay success and FPS are unverified. See Character-Diffuse-R422-validation.md for the capture-based approximation and limitations.

- Runtime commit: 2e48b557779000b208d4728eeea8a1d0a7f69235
- Workflow: https://github.com/Frooglemcdoo/Control-FG/actions/runs/42947447450
- Job: 130283537793; all 60 steps succeeded.
- Artifact: 13824495047
- Outer artifact SHA256: d9e12a7cd4a63c974ffed4a0ace506f6436f70ae2bf3723f853f148bc48103ac
- Package: ControlTOH-R422.zip; 6795695 bytes; 83 entries.
- Package SHA256: 9920ea67389a0ba844df3442e2d68703a45da972da034c7c18a61b171ba8f813
- DLL SHA256: 562a3e6e8bd8f892b7bb518462fea0e8ec3a1798717399003230b10ae8285832
- Download: https://chatgpt.com/library?file_id=libfile_9f5460f8f708819182246260396f4820&account_id=personal

Verified outer and inner hashes, ZIP integrity, x64 PE, R422 startup/UI markers, explicit surface_weights_no_radiance_veto marker, retained CAPTURE MOTION and RELAX controls, unchanged character guide/history markers and packaged instructions.

Windows logs confirm 384 emitted reconstruction cases, 36 selected-coverage cases, three periodic energy fixtures, and both sets of eight real WARP dispatch/readback cases (filter and guarded filter). Native remap and preserved non-filter character outputs also pass.

Install with game closed. Keep Enhanced Character Lighting ON, RELAX OFF and RR ON; native GI ray count is unchanged. Evaluate the same moving jaw/neck and hair views, including skin brightness, strand detail and local character shadow edges. No new capture required initially.

The earlier superseded run used a mismatched shell check in the workflow; this package is from the corrected head above. No additional rendering change was made for that correction.

