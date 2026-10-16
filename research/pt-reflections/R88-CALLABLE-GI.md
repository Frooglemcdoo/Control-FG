# Separate terminal GI callable shader

The R87 capture records two cache snapshots (11 volumes, scale1) and then
PT_PRIMARY_PIPELINE_FAILED hr=0x887A0005. No private PT dispatch was submitted.
Pipeline::Initialize contains local-root creation, state-object creation and
identifier lookup, so the old single HRESULT cannot prove which API first failed
or whether an earlier asynchronous GPU operation removed the device. Do not
claim a GI traversal fault, OOM, or confirmed NVIDIA compiler bug.

R87 added the entire tree traversal and brick sampler to the ray-generation
shader. WARP testing exercised that code as a compute shader, not the driver's
DXR pipeline compilation. R88 keeps the GI calculation but isolates it in the
PTTerminalGICallable export. Microsoft specifies that callable shaders compile
separately from their callers. This targets the new monolithic pipeline structure;
it is a corrective hypothesis, not a demonstrated crash diagnosis.

The path loop records terminal position/normal and throughput*kd, then ends.
After the loop, one CallShader(0, parameters) computes the cache term. Its result
is multiplied by the saved weight and enters the existing primary-lobe split.
No further TraceRay runs after the call. The36-byte call parameter structure is
shared by caller/callee. The helper has no local root or hit-record arguments.
The global root and resource bindings are unchanged from R87. The driver's default
pipeline stack sizing is retained; callable nesting is one and it traces no rays.

The optional32-byte callable record follows the hit table at128+N*128, aligned to
64 bytes with64 reserved bytes. Existing hit offsets/strides and scene leases are
unchanged. Legacy additive PT has no callable. Both identifiers and the callable
record are required for surface PT initialization; there is no silent no-GI fallback.

Initialize now records the precise stage, device reason before state creation and
after failure, and a successful private-pipeline marker. No debug layer is forced
on and no global device settings are changed. This is evidence capture alongside
the shader correction, not a substitute for it.

Validation: production DXIL compile, exported callable/callShader contract,
C++ table alignment/bounds, unchanged GI compute arithmetic on WARP, and all
existing regression gates. These do not validate creation on the user's NVIDIA
driver, so do not claim the crash is confirmed fixed. R87 resource state/lifetime
questions still require live integration evidence before treating GI as complete.

References:
https://learn.microsoft.com/en-us/windows/win32/direct3d12/callshader-function
https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html
https://developer.nvidia.com/blog/parallel-shader-compilation-ray-tracing-pipeline-states/
