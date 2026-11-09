#pragma once
#include <cstdint>
#include <limits>
namespace PTGlass {
constexpr std::uint64_t HitStride=64;
constexpr unsigned SlotCount=3;
enum class Phase { Empty, Idle, Recording, Submitted, Quarantined };
// Hold a slot across the native ray submission until its lighting consumer
// has been recorded and submitted. Producer completion alone is insufficient.
struct SubmissionHold {
    bool submitted=false,anySubmitted=false,finalized=false;
    void Submitted() noexcept {submitted=true;anySubmitted=true;}
    bool Finish(bool differentConsumer) noexcept {
        if(differentConsumer&&!submitted)return false;
        finalized=true;if(differentConsumer)submitted=false;return true;
    }
    bool Ready() const noexcept {return finalized&&submitted;}
};
struct Ticket {
    Phase phase=Phase::Empty;
    std::uint64_t recording=0, fence=0;
    bool Lease(std::uint64_t id) noexcept {
        if(!id || (phase!=Phase::Empty && phase!=Phase::Idle))return false;
        phase=Phase::Recording;recording=id;fence=0;return true;
    }
    bool Submit(std::uint64_t id,std::uint64_t value) noexcept {
        if(phase!=Phase::Recording || recording!=id || !value || value==UINT64_MAX)return false;
        phase=Phase::Submitted;fence=value;return true;
    }
    bool Discard(std::uint64_t id) noexcept {
        if(phase!=Phase::Recording || recording!=id)return false;
        phase=Phase::Idle;recording=0;return true;
    }
    bool Complete(std::uint64_t value) noexcept {
        if(value==UINT64_MAX)return false; // D3D12 device-removed sentinel is not completion.
        if(phase!=Phase::Submitted || value<fence)return false;
        phase=Phase::Idle;recording=0;return true;
    }
    bool Releasable() const noexcept {return phase==Phase::Empty || phase==Phase::Idle;}
};
inline bool HitBytes(std::uint32_t width,std::uint32_t height,std::uint64_t* bytes) noexcept {
    if(!bytes)return false;
    *bytes=0;
    if(!width || !height || width>16384 || height>16384)return false;
    *bytes=std::uint64_t(width)*height*HitStride;
    return true;
}
}
