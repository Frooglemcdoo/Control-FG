#pragma once
#include <cstdint>
namespace native_prep {
using Tick=unsigned long long;
constexpr unsigned kPhases=5;
inline const char* Name(unsigned i) noexcept {
    constexpr const char* names[]={"query_before_readback","readback_and_processing","scene_selection_and_assembly","prebuild_sizing","buffer_management_and_upload"};
    return names[i];
}
struct Timeline {
    Tick begin=0,map=0,unmap=0,estimateBegin=0,estimateEnd=0,end=0;
    bool readback=false,unmapped=false,estimated=false,estimateComplete=false;
    bool Split(Tick (&out)[kPhases]) const noexcept {
        if(!estimated||!estimateComplete||estimateBegin<begin||estimateEnd<estimateBegin||end<estimateEnd)return false;
        if(readback!=unmapped)return false;
        if(readback&&(map<begin||unmap<map||estimateBegin<unmap))return false;
        out[0]=readback?map-begin:0;
        out[1]=readback?unmap-map:0;
        out[2]=estimateBegin-(readback?unmap:begin);
        out[3]=estimateEnd-estimateBegin;
        out[4]=end-estimateEnd;
        return true;
    }
};
}
