# R88 evidence and R89 corrective integration

Capture PT-Primary-M12R88-Results-20261016-140035-179 confirms commit
fbd2ba9ef19126f7302ef16cc0fd8357298d40f7. At frame3182 pipeline creation reports
hr=0x887A0005 stage=create_state_object device_before=0x00000000
device_after=0x887A0020 terminal_gi_callable=1. No private PT dispatch follows.
The failure is localized to the CreateStateObject interval, not GI execution.
An asynchronous native failure remains possible; neither OOM nor a specific
compiler defect is proven. Microsoft's DXGI_ERROR documentation identifies
0x887A0020 as DXGI_ERROR_DRIVER_INTERNAL_ERROR.

R89 removes GI evaluation from the DXR compilation unit, including CallShader,
and keeps the GI feature as an ordinary compute shader. It creates that PSO with
the same native global root; creation failures have a separate log. DXR only
writes a terminal request, then compute adds the weighted GI tail before filtering.
The terminal diffuse coefficient and accumulated throughput are multiplied once.
The primary first-lobe diffuse fraction partitions the contribution per channel;
the complementary fraction goes to specular. Raw guide fields remain unchanged.

Two 32-byte/pixel reconstruction buffers already owned by ReconstructionPass
temporarily carry requests at u4/u5 (private slots45/46). The first holds
position.xyz/valid and normal.xyz; the second holds throughput*kd and fraction.
These are distinct current-frame buffers, never previousSignals. No additional
image allocation, compression or CPU readback is introduced. Existing budgets
and fence-owned Assets lifetime apply. One extra compute dispatch is required.

Recording order: transition requests NON_PIXEL_SHADER_RESOURCE to UAV; trace;
UAV barrier; GI compute; return request buffers to NON_PIXEL_SHADER_RESOURCE;
restore native PSO/table/camera; existing raw-output barrier/transition; existing
reconstruction transitions/overwrites the temporary contents. GI compute uses
private b5 integer width/height in previously unused last8 bytes. No root change.

Admission0/2 returns before accessing request/radiance resources. Every admitted
raygen pixel clears the request-valid word before material/depth rejection.
Compute requires request validity AND raw coverage, exact buffer sizes, finite
weights/fractions, and GI admission. Disabled/outside/malformed volumes retain
the already computed PT signal. Sparse probe results cannot consume stale data.

The WARP test compiles the production compute source and validates nontrivial
RGB throughput/fractions, opposite normals, outside volumes, admission0/2,
disabled GI, extent mismatch, inactive requests with stale payload, invalid
weights/fractions, zero weights, pure specular attribution, and preservation of
all guide/coverage words. Existing traversal tests remain. Production DXIL checks
reject GI textures/callables in raygen and require the request UAV bindings.
These tests do not establish NVIDIA runtime success or complete lighting fidelity.

Reference: https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/dxgi-error
