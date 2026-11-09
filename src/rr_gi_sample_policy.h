#pragma once
namespace control_rr {
// R33: explicit UI choices override valid native counts; Native preserves them.
// Audited native tweakable range is 1..16. Invalid values remain untouched.
inline unsigned NormalizeGISampleRequest(unsigned requested) noexcept {
 return requested>=2 && requested<=16 && requested%2==0 ? requested : 0u;
}
inline unsigned NextGISampleRequest(unsigned current) noexcept {
 // Comparison button: Native -> 2 -> 4 -> Native.
 return current==0 ? 2u : current==2 ? 4u : 0u;
}
inline unsigned GISampleComparison(unsigned nativeCount,unsigned requested,bool fullRR) noexcept {
 const unsigned samples=NormalizeGISampleRequest(requested);
 return fullRR&&samples&&nativeCount>=1&&nativeCount<=16 ? samples : nativeCount;
}
}
