// Compile the actual Windows boundary reader and SEH leaves independently of
// the large proxy build. Native services are declarations only: this target
// must never be linked or presented as an execution/ABI test.
#include <windows.h>
#include <d3d12.h>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>
extern HMODULE verifiedRenderer,verifiedD3d;
extern std::atomic<unsigned long long> beginCount,presentCount;
extern const std::uintptr_t kEngineFrameCounterRva;
void Log(const char*,...);
bool ReadEngineFrameSafe(unsigned long long*,DWORD*) noexcept;
#include "../../src/camera_capture.h"
#include "../../src/rr_specular_native_clamp.h"
#include "../../src/rr_provider_auth.h"
namespace control_rr_reflection {
struct WindowsAccess {
 static bool Read(void*,std::uintptr_t,void*,std::size_t) noexcept;
};
}
bool RRProviderEntry(void*,std::int32_t,control_rr_provider::Entry&) noexcept;
bool RRProviderCompare(void*,std::int32_t,const void*,int&) noexcept;
bool RRProviderReadBound(void*,std::int32_t,void*,std::size_t) noexcept;
#include "../../src/nrd_diffuse_native_inputs.h"
bool CompileNativeInputReader(const control_rr_native_clamp::Api& api,UINT64 frame,UINT64 view,
                             control_nrd_native::Inputs& out,const char*& reason) {
 return control_nrd_native::Read(api,frame,view,out,reason);
}
