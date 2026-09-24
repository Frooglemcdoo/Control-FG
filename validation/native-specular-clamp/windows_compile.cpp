#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <intrin.h>
#include <cstdint>
static HMODULE verifiedD3d=nullptr, verifiedRenderer=nullptr;
#include "../../src/rr_specular_native_clamp.h"
int main(){
 control_rr_native_clamp::Api api;
 control_rr_native_clamp::Lease lease;
 return api.Initialize(nullptr,nullptr)||api.Arm(lease)||api.StillForced(lease)||api.Restore(lease)?1:0;
}
