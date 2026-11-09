#pragma once
#include <cmath>
#include <cstdint>
namespace control_rr {
// Value-only metadata from this native evaluation invocation. No borrowed
// pointers survive the gateway. Jitter is retained in native NGX pixel units.
struct LiveFrameInput {
    unsigned long long frame=0;
    float jitterX=0,jitterY=0;
    int reset=0;
    unsigned valid=0;
    unsigned long fault=0;
    unsigned width=0,height=0;
    bool MatchesExtent(unsigned w,unsigned h) const noexcept {return width!=0 && height!=0 && width==w && height==h;}
    bool Matches(unsigned long long frameNow) const noexcept {
        return frame!=0 && frame==frameNow && fault==0 && (valid&0x70u)==0x70u &&
            std::isfinite(jitterX) && std::isfinite(jitterY);
    }
};
}
