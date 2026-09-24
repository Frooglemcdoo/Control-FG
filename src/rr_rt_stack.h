#pragma once
#include <cstdint>
#include <cstddef>
namespace control_rr {
// Preserve the original six diagnostic bits; mesh particles/debris is bit 6.
enum RTEffect : unsigned {RTReflection=1,RTDiffuseGI=2,RTSunShadow=4,
 RTContactShadow=8,RTAO=16,RTTransparent=32,RTDebris=64};
constexpr unsigned RTKnownEffects=127;
constexpr std::size_t RTOptionValues[]{0x912d00,0x912df0,0x913150,0x913060,0x9133c0,0x914260,0x90e9d0};
inline bool RRSupportsRTSettings(unsigned effects) noexcept {
 // Reflection capture remains the source of specular hit distance.
 return (effects&RTReflection)!=0&&(effects&~RTKnownEffects)==0;
}
// Evidence for the native GI bypass is scoped to one selected frame. The exact
// call being observed releases GI history AFTER the raw GI producer and skips
// filterDiffuseNoise. Other RT paths continue through their native shaders.
struct RRRTFrame {
 std::uint64_t frame=0;unsigned effects=0;bool selected=false,giObserved=false;
 void Begin(std::uint64_t f,unsigned e,bool rr) noexcept {frame=f;effects=e;selected=rr;giObserved=false;}
 bool ObserveGI(std::uint64_t f,bool lightingStarted,bool correctHistory,bool emptyHistory) noexcept {
  if(!selected||!(effects&RTDiffuseGI)||f!=frame||!lightingStarted||!correctHistory||!emptyHistory||giObserved)return false;
  giObserved=true;return true;
 }
 bool Complete(std::uint64_t f,unsigned currentEffects,bool reflectionComplete) const noexcept {
  return selected&&f==frame&&effects==currentEffects&&RRSupportsRTSettings(effects)&&reflectionComplete&&
   (!(effects&RTDiffuseGI)||giObserved);
 }
};
}
