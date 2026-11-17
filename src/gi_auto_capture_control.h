#pragma once
#include "gi_reuse/auto_capture_region.h"
#include <atomic>
namespace gi_auto_capture {
enum Phase:unsigned {Preview,Countdown,Collecting,Complete};
inline std::atomic<unsigned> phase{Preview},center{200u|(500u<<16)},frozenCenter{200u|(500u<<16)};
inline std::atomic<std::uint64_t> dimensions{0},request{0},recorded{0};
inline std::atomic<unsigned> completed{0};
inline gi_reuse::GIAuditRect Region(unsigned w,unsigned h){
 const auto p=phase.load();const unsigned c=(p==Countdown||p==Collecting)?frozenCenter.load():center.load();
 return gi_reuse::GIAuditRegion(w,h,c&65535u,c>>16);
}
}
