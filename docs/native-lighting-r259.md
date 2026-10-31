# R259: isolate the second GI history blend

Use Native Blend, RR ON, FG OFF. In F10 cycle the GI button from Clamp Only to
**GI: BOTH TEST**, close F10, wait ten seconds, and repeat the movement that
shows ghosting. Compare against **GI: BOTH**, waiting ten seconds after switching.
Report trails, speckles, shadow boiling, lighting changes and performance. Send
`ControlTOH/Tools/Collect-ControlFG-Compact-Logs.cmd` output. No material capture
or full RR image capture is needed for this first visual test.

The accepted Clamp Only default and original Both remain available. The test
mode is not persisted. DirectStorage, foliage admission and other lighting
features are unchanged. This is an isolation experiment, not a confirmed fix.

## Corrected diagnosis

The two native spatial filter variants in deferredlight_filtering_diffuse.obj
also perform temporal accumulation on the final spatial pass. The branch checks
`filterPass + 1 == filterPassCount`, then samples g_DLF_tOutputHistory and blends
with up to 70% history using native depth/motion tests. Earlier R210–R213 patches
affected only the initial temporal shader. In particular the old Spatial Only
label means the INITIAL temporal weight was zero; it did not remove this final
history blend. Earlier claims of no history anywhere in Spatial Only were wrong.

R259 keeps the R212 adaptive initial temporal pass and all six native spatial
passes. Only in Both Test, the two exact native spatial shaders are replaced
with variants whose final history coefficient is zero. Each differs at one
SHEX DWORD (1962, 0.7 to 0), plus the DXBC checksum. Spatial taps, material/depth
edge weighting, resources, root signature and output ABI remain unchanged.
The native history copy and its timing remain unchanged; no clear or early
history overwrite is introduced. Overwriting history before spatial filtering
would be incorrect because those shaders still read the old history.

The validated renderer spatial dispatch callsite is RVA 0x15a28. Replacement
is scoped to the existing eligible native GI filter, matching command-list/frame
context, successful initial adaptive dispatch, and the latched Both Test mode.
Unknown bytecode and unavailable PSOs retain native work. Original PSOs and scope
flags are restored after dispatch, including exception paths. Log
`gi_single_history build=R259 ... complete=1` requires one temporal replacement
and the expected count of spatial replacements. Fallback is not a valid test.

## Validation

Source tests verify both full source hashes, the single changed instruction
operand, unchanged other DXBC chunks and bindings, and unknown-source rejection.
Windows WARP tests execute actual spatial shader bytecode over eight-frame
history feedback with bright/dark initial history, nonzero velocity, last and
non-last passes, and CameraCut. They check interior RGBA output (excluding native out-of-bounds stencil edges) and exercise the
production PSO selection. Native-scope tests cover five modes, context rejection,
fallback and exceptions. Existing temporal tests remain in the full CI suite.

These synthetic fixtures isolate the final spatial history blend; they are not
a reproduction of the full six-pass scene or RR. In-game noise suppression and
ghosting are still to be tested. Removing the second temporal blend may expose
noise even with the initial temporal stage retained.
