#pragma once
namespace control_rr {
inline bool UseNativeSignalClamp(bool contaminated,bool prepared,unsigned strength=100) noexcept {
 return strength!=0&&!contaminated&&prepared;
}
}
