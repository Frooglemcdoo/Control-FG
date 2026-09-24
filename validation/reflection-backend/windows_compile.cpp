#include "../../src/rr_reflection_windows_access.h"
// Explicit instantiation compiles all backend/coordinator methods against the
// actual Windows SDK. This is a compiler gate, not a runtime/GPU test.
template class control_rr_reflection::Backend<control_rr_reflection::D3D12Api>;
template class control_rr_reflection::Capture<control_rr_reflection::D3D12Backend>;
