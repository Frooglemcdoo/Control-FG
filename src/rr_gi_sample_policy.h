#pragma once
namespace control_rr {
// R29 comparison: never lower a user/native count or override unknown values.
inline unsigned GISampleComparison(unsigned nativeCount,unsigned requested,bool fullRR) noexcept {
 return fullRR&&requested==2&&nativeCount==1 ? 2u : nativeCount;
}
}
