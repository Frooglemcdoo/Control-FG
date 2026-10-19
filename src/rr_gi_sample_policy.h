#pragma once
namespace control_rr {
// R31 comparison: never lower a user/native count or override unknown values.
inline unsigned NormalizeGISampleRequest(unsigned requested) noexcept {
 return requested==2||requested==4||requested==6||requested==8 ? requested : 0u;
}
inline unsigned GISampleComparison(unsigned nativeCount,unsigned requested,bool fullRR) noexcept {
 const unsigned samples=NormalizeGISampleRequest(requested);
 return fullRR&&samples&&nativeCount==1 ? samples : nativeCount;
}
}
