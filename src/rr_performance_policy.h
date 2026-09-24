#pragma once
#include <cstdint>
#include <cstddef>
#include <limits>
namespace control_rr_perf {
enum class State { Free, Recording, Recorded, Submitted };
struct Ticket { std::size_t index=256; std::uint64_t serial=0; };
struct Slot {
    State state=State::Free;
    std::uint64_t serial=0,frame=0,present=0,fence=0;
    bool resolved=false;
};
template<std::size_t N> struct Pool {
    Slot slots[N]{};std::uint64_t serial=0;
    bool Begin(std::uint64_t frame,std::uint64_t present,Ticket& out) noexcept {
        out={};
        for(std::size_t i=0;i<N;++i)if(slots[i].state==State::Free){
            slots[i]={State::Recording,++serial,frame,present,0,false};out={i,serial};return true;
        }
        return false;
    }
    bool End(Ticket t,bool resolved) noexcept {
        if(t.index>=N||slots[t.index].serial!=t.serial||slots[t.index].state!=State::Recording)return false;
        slots[t.index].resolved=resolved;slots[t.index].state=State::Recorded;return true;
    }
    bool Submit(std::size_t i,std::uint64_t returnedPresent,std::uint64_t fence) noexcept {
        if(i>=N||slots[i].state!=State::Recorded||!fence||slots[i].present==std::numeric_limits<std::uint64_t>::max()||
           returnedPresent!=slots[i].present+1)return false;
        slots[i].state=State::Submitted;slots[i].fence=fence;return true;
    }
    bool Completed(std::size_t i,std::uint64_t value) const noexcept {
        return i<N&&value!=std::numeric_limits<std::uint64_t>::max()&&slots[i].state==State::Submitted&&value>=slots[i].fence;
    }
    bool Retire(std::size_t i,std::uint64_t value) noexcept {
        if(!Completed(i,value))return false;
        slots[i]={};return true;
    }
};
inline bool Milliseconds(std::uint64_t begin,std::uint64_t end,std::uint64_t timestampFrequency,double& ms) noexcept {
    if(!timestampFrequency||end<begin)return false;
    ms=1000.0*static_cast<double>(end-begin)/static_cast<double>(timestampFrequency);
    return ms<=10000.0;
}
struct Key {
    unsigned width=0,height=0,outputWidth=0,outputHeight=0,fg=0,hdr=0,effects=0;
    bool rr=false,requested=false,ready=false;
    bool operator==(const Key& b) const noexcept {
        return width==b.width&&height==b.height&&outputWidth==b.outputWidth&&outputHeight==b.outputHeight&&
            fg==b.fg&&hdr==b.hdr&&effects==b.effects&&rr==b.rr&&requested==b.requested&&ready==b.ready;
    }
};
struct Stability {
    Key key{};std::uint64_t lastFrame=0,lastTicks=0,stableSince=0,segment=0;bool have=false;
    bool Observe(const Key& next,std::uint64_t frame,std::uint64_t ticks,std::uint64_t timestampFrequency,
                 bool reset,bool success,double& cadence) noexcept {
        cadence=0;
        const bool consecutive=have&&frame==lastFrame+1&&ticks>lastTicks&&key==next;
        if(consecutive)Milliseconds(lastTicks,ticks,timestampFrequency,cadence);
        const bool transition=!consecutive||reset||!success||cadence>250.0;
        if(transition){stableSince=ticks;++segment;}
        key=next;lastFrame=frame;lastTicks=ticks;have=true;
        return !transition&&success&&next.ready&&timestampFrequency&&ticks>=stableSince&&
            static_cast<double>(ticks-stableSince)/static_cast<double>(timestampFrequency)>=3.0;
    }
};
}
