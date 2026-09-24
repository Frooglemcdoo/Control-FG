#pragma once
namespace control_rr {
inline bool DistanceEligible(bool ready,bool projection,unsigned active,unsigned presetF) noexcept {
 return ready&&projection&&active==presetF;
}
struct DistanceBindingHistory {
 bool bound=false;
 bool Update(bool next) noexcept {const bool changed=bound!=next;bound=next;return changed;}
};
}
