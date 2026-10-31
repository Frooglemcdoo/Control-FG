#pragma once
#include "texture_streaming_budget.h"
namespace storage_startup_policy {
// Set only after the verified runtime installs; never inferred from an env flag.
inline bool active=false;
inline int pinnedSlices=0;
constexpr bool Flag(const char* text,unsigned length,bool fallback) noexcept {
    return !length?fallback:text&&length==1&&text[0]=='1';
}
#ifdef _WIN32
inline bool EnvironmentFlag(const char* name,bool fallback) noexcept {
    char text[8]{};
    return Flag(text,GetEnvironmentVariableA(name,text,sizeof(text)),fallback);
}
#endif
struct Settings {float budget;int slices;};
inline Settings Resolve(unsigned requested,bool installed,int pinned) noexcept {
    requested=texture_streaming_budget::Normalize(requested);
    return {requested?static_cast<float>(requested):installed?4.0f:1.0f,
        pinned?pinned:(requested||installed?16:8)};
}
}
