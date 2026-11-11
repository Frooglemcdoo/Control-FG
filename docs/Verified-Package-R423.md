# Verified R423 diagnostic package

R422 was rejected for worse character artifacts. R423 restores the four character
diffuse implementation/test files exactly from R421 (84d6df15c5f5b90854f31174baf418e9576807ef).
It adds the audited native reconstruction map to motion capture. It does not
claim to fix the original motion-dependent jaw/neck noise or red hair speckles.

- Runtime commit: 9947a02254948f5abf5e5fd9bd0f5820c32227f1
- Workflow: https://github.com/Frooglemcdoo/Control-FG/actions/runs/43012792448
- Job: 130496310951; all 60 steps succeeded.
- Artifact: 13852120986
- Package: ControlTOH-R423.zip; 6,798,358 bytes; 83 entries.
- ZIP SHA256: bff90f64cf5259b4dfc4518031b67dc07a2438448b13e208b6c3169203ab978b
- dxgi.dll SHA256: 5154567c6d003ac5efd352164688d377439d0ee98e7bf04ea41b573f83a66039
- Outer artifact SHA256: efc6e28990a676d3a3cdbf2c789b6502235677e1b4c4b9cfd69577e994de17b0
- Saved package: https://chatgpt.com/library?file_id=libfile_8e308ea2d0c0819182506a7ddb864ea3&account_id=personal

Checked both archive CRCs and hashes, x64 PE, R423 heading/startup markers,
CAPTURE MOTION, restored-filter marker, map filename and diagnostic README.

Windows logs confirm 384 emitted reconstruction cases across 12 families,
eight WARP filter dispatch/readback cases, 6144 native remap cases, map capture
capacity/state wiring, full-resolution motion sequence, and 132 baseline files.
These are implementation checks, not in-game visual validation.

Remaining evidence needed: a moving GI-lit character capture containing
native-reconstruction-map.r32u. Earlier R421/R422 exports lack that buffer and
cannot supply it retrospectively. Preserve the previously accepted environment
appearance and do not resume radiance-veto removal from R422.

Limitations: map is captured at the RR boundary, not each character draw;
per-fragment coverage is absent. Map availability/state/format in the live game
still needs confirmation. See Reconstruction-Map-Audit-R423.md. No performance
gain or complete source-surface correspondence is claimed.
