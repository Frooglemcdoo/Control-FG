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
// GI evidence is scoped to one selected frame. Bypassed frames require the
// history release after the producer; filtered frames require native filtering
// to return with retained history. Neither path can satisfy the other.
struct RRRTFrame {
 std::uint64_t frame=0;unsigned effects=0;bool selected=false,giObserved=false,giFiltered=false;
 void Begin(std::uint64_t f,unsigned e,bool rr,bool filtered=false) noexcept {frame=f;effects=e;selected=rr;giObserved=false;giFiltered=filtered;}
 bool ObserveGI(std::uint64_t f,bool lightingStarted,bool correctHistory,bool emptyHistory) noexcept {
  if(giFiltered||!selected||!(effects&RTDiffuseGI)||f!=frame||!lightingStarted||!correctHistory||!emptyHistory||giObserved)return false;
  giObserved=true;return true;
 }
 bool ObserveFilteredGI(std::uint64_t f,bool lightingStarted,bool correctHistory,bool retainedHistory,unsigned passes) noexcept {
  if(!giFiltered||!selected||!(effects&RTDiffuseGI)||f!=frame||!lightingStarted||!correctHistory||!retainedHistory||!passes||giObserved)return false;
  giObserved=true;return true;
 }
 bool Complete(std::uint64_t f,unsigned currentEffects,bool reflectionComplete) const noexcept {
  return selected&&f==frame&&effects==currentEffects&&RRSupportsRTSettings(effects)&&reflectionComplete&&
   (!(effects&RTDiffuseGI)||giObserved);
 }
};
}
