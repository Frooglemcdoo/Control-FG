#pragma once
#include <atomic>
namespace control_relax {
enum class Profile : unsigned { Original, Balanced };
inline std::atomic<Profile> requested{Profile::Balanced};
inline const char* Name(Profile p) noexcept {return p==Profile::Balanced?"balanced":"original";}
inline bool Valid(Profile p) noexcept {return p==Profile::Original||p==Profile::Balanced;}
// Explicit values match pinned NRD 4.18 defaults in Original. Balanced reduces
// spatial reuse; no ray count, temporal lifetime or output sharpening changes.
template<class Settings> inline void Spatial(Settings& s,Profile p) noexcept {
 s.diffusePrepassBlurRadius=p==Profile::Balanced?12.f:30.f;
 s.atrousIterationNum=p==Profile::Balanced?3u:5u;
 s.diffusePhiLuminance=2.f;
}
// manualGI=4 is the existing raw-GI bypass; other manual comparisons reset.
inline unsigned Cycle(unsigned manual) noexcept {
 if(manual){requested=Profile::Balanced;return 0;}
 if(requested.load()==Profile::Balanced){requested=Profile::Original;return 0;}
 return 4;
}
}
