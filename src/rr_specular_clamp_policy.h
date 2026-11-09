#pragma once
namespace control_rr_specular_policy {
enum class Path { Reference, NativeClamp, RawCopy };
// The bind-stage decision is sticky for a dispatch. If a native CameraCut lease
// was armed because the optional reference was not ready at bind time, do not
// switch to the reference shader mid-pass.
inline constexpr Path Choose(bool referenceRequested,bool referenceReady,bool nativePrepared) noexcept {
 if(referenceRequested&&referenceReady&&!nativePrepared)return Path::Reference;
 if(nativePrepared)return Path::NativeClamp;
 return Path::RawCopy;
}
}
