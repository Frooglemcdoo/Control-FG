# R430 unguided recovery accounting

R429 logs at 20261112-104716: see r429-session-summary.json. All sampled
consumers 1-4 and600 had guided=0 native_complement=1. No guided success
observed. Source budget approaches768MiB; eight E_OUTOFMEMORY records do not
prove exhausted5090 VRAM. No exact per-pass GPU timing is available.

R429 incorrectly reset its rejection streak at producer success and did not
count NativeShadeRequired. R430 resets only on Recorded guided completion;
all rejected attempts and unguided recoveries count toward eight, then stop
new guided work/source registration. Existing owners retain retirement safety.

GuidedRejection supplies exact guided-only failure. Output diagnostics preserve
existing acceptance and report stage, native flags/alias, tracked state,
resource dimensions/mips/samples/flags. Diagnosis repeats admission only for
bounded unguided recovery, never on successful guided frames.

Native exact binaries independently confirm renderer90E9F8 is DiffuseGITarget,
133A0 dereferences its handle, and D3D EA80 reads container+8. Native12C621
and12C68B prepare/bind it for GI6. This supports the selected container; it does
not establish the live output flags/state or justify bypassing those checks.

No sampling/RELAX/RR change, no increased allocation budget, no measured FPS
recovery or confirmed noise fix. The exact guided-only rejection still requires
runtime evidence. Do not repeat earlier R425-427 scene/ordering captures.

Final source9348856d42d95ee5a2af307f426ae8e1a0851bc3. Windows core43264038663/job131342725522 SUCCESS;
full package43264038433/job131342828908 SUCCESS. Initial f8700e2f build cancelled.

Verified artifact13964654404; package 6992328 bytes, 88 members.
ZIP SHA256: 0232f5925fa0f793cd0118f4e1a54362cee4d7d453dddf903856f043bd22a3e5
DLL SHA256: c299ecb3d358cc5db5e6c51741456f491dc68b9a1c45d79e20dba6a43a7e0da3
Launcher and runtime markers verified.
Next required evidence: briefly load through Start-GI-Frozen-R430.cmd and
collect existing compact logs. No video/PIX or repeated ordering capture.
