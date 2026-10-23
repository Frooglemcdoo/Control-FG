# R127: native hit-pipeline preparation

R126 captured a complete, submission-ordered table with 12,752 records: 5,921
opaque identifiers, 455 alpha identifiers, 6,376 zero identifiers and no unknown
identifiers. This build prepares the separate pipeline needed to trace actual
hit identities. It does not yet dispatch it or fix the lighting defect.

## Test

Close Control to install beside Control_DX12.exe. Preserve NVIDIA runtimes and
ControlFGStreamline. Use the same affected room, RT Contact Shadows on, samples 1,
FG and glass enhancements off. Select BOTH, run for 70 seconds, SAVE LOG and send
Lighting-R127. No screenshots or mode restarts are required.

## Implemented

The native spotlight pipeline's original library supplies only its alpha-test
any-hit export to a separate diagnostic state object. Dedicated closest-hit,
miss and raygeneration shaders use a 32-byte payload and 8-byte triangle attributes.
The native global root and local material root are retained. Gameplay state,
shader configuration, ray records and outputs are not modified by preparation.
A rejected diagnostic state logs its HRESULT and leaves gameplay operational.

Capture identifiers belong to that independent state object. They are attached
to its matching gameplay owner and recorded with each observed table dispatch.
On table validation, an allocation-free translator writes to a private CPU buffer.
It replaces known nonzero identifiers, preserves all 64 trailing bytes per record,
and preserves zero-identifier records byte-for-byte, including nonzero trailing
arguments. It never compacts or reorders records. Unknown identifiers reject the
whole translation before writing. No GPU upload or diagnostic dispatch occurs.

Look for rt_hit_pipeline ready=1 and rt_hit_table_translation ready=1. These prove
native-driver state-object creation and CPU table preparation only. WARP tests
validate descriptor construction and failure handling, not native DXR traversal.
The shader is compiled as lib_6_5 and embedded in the DLL during the build.

## Still required for hit geometry

Capture the exact native centered-ray inputs, bind same-generation TLAS and private
input/output descriptors, record bounded diagnostic work, restore every native
binding, and export results only after the exact submitted queue's fence completes.
The prepared table alone must not be reused against a later unrelated scene.

Existing centered shadows, GI filtering, material guides, glass and 60% clamp are
preserved. Startup fence handling and table capture remain from R126.
