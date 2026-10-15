#pragma once
#include <array>
#include <cstdint>
namespace PTScalingMetrics {
// Completed, armed, non-probe PT dispatches only. CPU and GPU values overlap
// with other work and must never be summed into a purported frame time.
struct Window {
    unsigned width=0,height=0,samples=0;
    double gpu=0;std::array<double,6> cpu{};
    std::uint64_t sequence=0;
    bool Add(unsigned w,unsigned h,double ms,const std::array<double,6>& costs) {
        if(!w||!h||ms<0)return false;
        if(w!=width||h!=height){width=w;height=h;samples=0;gpu=0;cpu={};++sequence;}
        ++samples;gpu+=ms;for(unsigned i=0;i<cpu.size();++i)cpu[i]+=costs[i];
        return samples==120;
    }
    void ClearSamples(){samples=0;gpu=0;cpu={};}
};
}
