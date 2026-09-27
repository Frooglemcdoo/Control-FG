# PT0 — Fat DXR Probe

This is the single comprehensive path-tracing discovery build. It does not replace Control's rendering yet.

## Automatic capture
The first 64 native DeviceUtilRaytracing dispatches are captured in detail. After that PT0 keeps pass counts and periodic summaries.

PT0 records:
- descriptor heaps from device creation
- CBV/SRV/UAV descriptor metadata and descriptor copies
- ID3D12GraphicsCommandList4 BuildRaytracingAccelerationStructure
- SetPipelineState1 state objects
- DispatchRays dimensions and raygen/miss/hit/callable shader tables
- compute root signature, root descriptor tables, direct CBV/SRV/UAV GPU addresses and constants hashes
- TLAS matches from both root SRV and RTAS descriptors
- pass fingerprints/classification
- known Control reflection/DGI/GBuffer/light resources

## F7
Press **F7 once in the scene we care about**. This arms a 600-Present detailed capture window. Move the camera through the noisy area and let it run for roughly 10 seconds, then collect the compact logs.

Key markers:
- PT0_READY
- PT0_DEVICE_HOOKS
- PT0_COMMAND_HOOKS
- PT0_BUILD_AS
- PT0_DXR_PASS
- PT0_DISPATCH_CAPTURE
- PT0_ROOT_BIND / PT0_ROOT_TABLE / PT0_DESCRIPTOR
- PT0_TLAS_OK
- PT0_RESOURCE_CENSUS
- PT0_PASS_SUMMARY
- PT0_SUMMARY

Success for PT0 is enough evidence to identify Control's RT passes, stable TLAS binding, state object/shader-table layout, descriptor resources, and the first pass we will replace with our own DXR state object in PT1.
