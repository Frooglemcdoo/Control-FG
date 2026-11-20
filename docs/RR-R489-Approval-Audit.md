# R489 restoration audit

Reference: pre-REBLUR commit 57bcc53. R488 recovery correction retained.

| Area | Restored behavior and evidence |
|---|---|
| RR activation | Two-state native Graphics control restored. Runtime status is reported separately from requested state. |
| Owned GI | RR owns the default independently of optional NRD filtering. Raygen, evaluator and capture replacement honor the same request. Native and replacement are alternatives, not additional dispatches. |
| Ray budget | Native/4/10 and other existing integer choices retained in one build. RR activation no longer overwrites the saved choice. |
| Clamp | Existing 60% reflection clamp implementation retained unchanged, with explicit native/bypass comparisons in F10. |
| Character, hair, material guides | Twelve rendering/streaming source hashes checked against pre-REBLUR source by test-rr-restored.py and its manifest. No claim that unresolved speckles are fixed. |
| REBLUR/RELAX | REBLUR compilation disabled. NRD live admission disabled for R489, preventing hidden RELAX substitution. Archived implementation remains in git. |
| F10 | Six controls and actual RR status; legacy test matrix, free-camera, F9 and numeric diagnostic shortcuts absent from this panel. |
| Capture | Existing R484 16-frame full-screen motion protocol and validators retained. Dedicated opt-in launcher, F10/Alt+Insert arm, STOP cue after frame four. |
| Idle capture | On-demand diagnostics retained, startup/resume semantic sampling gated by active capture. Full-scene resources require the capture launcher. |
| RR recovery | Native history reset precedes every filter route. Failed resets remain terminal. |

Validation includes standalone RR defaults without CONTROL_NRD_GI, persisted menu
routing, exact approved-source hashes, existing clean baseline checks and the
Windows build/GPU integration suite. In-game quality and FPS still require the
user's hardware; synthetic tests are not visual acceptance of Jesse's motion.
