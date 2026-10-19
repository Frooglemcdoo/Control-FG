#pragma once
namespace control_rr {
// R33: explicit UI choices override valid native counts; Native preserves them.
// Audited native tweakable range is 1..16. Invalid values remain untouched.
inline unsigned NormalizeGISampleRequest(unsigned requested) noexcept {
 return requested==2||requested==4||requested==6||requested==8 ? requested : 0u;
}
inline unsigned GISampleComparison(unsigned nativeCount,unsigned requested,bool fullRR) noexcept {
 const unsigned samples=NormalizeGISampleRequest(requested);
 return fullRR&&samples&&nativeCount>=1&&nativeCount<=16 ? samples : nativeCount;
}
}
