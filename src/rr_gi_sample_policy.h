#pragma once
namespace control_rr {
// R33: explicit UI choices override valid native counts; Native preserves them.
// Audited native tweakable range is 1..16. Invalid values remain untouched.
inline unsigned NormalizeGISampleRequest(unsigned requested) noexcept {
 return requested>=2 && requested<=16 ? requested : 0u;
}
inline unsigned NextGISampleRequest(unsigned current) noexcept {
 return current<2 || current>=16 ? 2u : current+1u;
}
inline unsigned GISampleSliderValue(unsigned position) noexcept {return position>=1 && position<=15 ? position+1 : 0u;}
inline unsigned GISampleSliderPosition(unsigned rays) noexcept {return NormalizeGISampleRequest(rays) ? rays-1 : 0u;}
inline unsigned GISampleComparison(unsigned nativeCount,unsigned requested,bool fullRR) noexcept {
 const unsigned samples=NormalizeGISampleRequest(requested);
 return fullRR&&samples&&nativeCount>=1&&nativeCount<=16 ? samples : nativeCount;
}
}
