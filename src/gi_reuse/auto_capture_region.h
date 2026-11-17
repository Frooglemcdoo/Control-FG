#pragma once
#include <algorithm>
#include <cstdint>
namespace gi_reuse {
struct GIAuditRect {std::uint32_t x=0,y=0,w=0,h=0;};
inline GIAuditRect GIAuditRegion(std::uint32_t w,std::uint32_t h,std::uint32_t cx,std::uint32_t cy){
 GIAuditRect r{};r.w=(std::min)(w,512u);r.h=(std::min)(h,512u);
 const auto x=std::uint32_t(std::uint64_t(w)*(std::min)(cx,1000u)/1000);
 const auto y=std::uint32_t(std::uint64_t(h)*(std::min)(cy,1000u)/1000);
 r.x=(std::min)(w-r.w,x>r.w/2?x-r.w/2:0u);r.y=(std::min)(h-r.h,y>r.h/2?y-r.h/2:0u);return r;
}
inline std::uint32_t GIAuditStepControl(std::uint32_t step,std::uint32_t x){return step|(x<<9);}
inline std::uint32_t GIAuditCapacityControl(std::uint32_t count,std::uint32_t y,bool enabled){return count|(enabled?65536u:0u)|(y<<17);}
}
