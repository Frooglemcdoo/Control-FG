#pragma once
#include <array>
#include <atomic>
#include <cstdint>
// A monotonic, conservative filter for the one-descriptor copy hot path.
// Bits are NEVER cleared: collisions, address reuse and saturation only send
// more work through the original synchronized observer. No metadata lives here.
namespace PTDescriptorInterest {
using U64=std::uint64_t;
class Filter {
    static constexpr unsigned Bits=24,Words=(1u<<Bits)/64;
    std::array<std::atomic<U64>,Words> words{};
    std::atomic<U64> first{UINT64_MAX},last{0};
    static U64 Hash(U64 address) noexcept {
        address^=address>>30;address*=0xbf58476d1ce4e5b9ull;
        address^=address>>27;address*=0x94d049bb133111ebull;
        return (address^(address>>31))&((1ull<<Bits)-1);
    }
public:
    Filter() noexcept {for(auto& word:words)word.store(0,std::memory_order_relaxed);}
    void Mark(U64 address) noexcept {
        const auto bit=Hash(address);
        words[bit/64].fetch_or(1ull<<(bit%64),std::memory_order_release);
    }
    bool Contains(U64 address) const noexcept {
        const auto bit=Hash(address);
        return (words[bit/64].load(std::memory_order_acquire)&(1ull<<(bit%64)))!=0;
    }
    // Retained mirror heaps never shrink. Enclosing gaps is conservative and
    // avoids a shared_ptr increment or a heap-map lock on every native copy.
    void IncludeMirror(U64 begin,U64 end) noexcept {
        auto value=last.load(std::memory_order_relaxed);
        while(value<end&&!last.compare_exchange_weak(value,end,std::memory_order_release,std::memory_order_relaxed)){}
        value=first.load(std::memory_order_relaxed);
        while(value>begin&&!first.compare_exchange_weak(value,begin,std::memory_order_release,std::memory_order_relaxed)){}
    }
    bool NeedsSimpleCopy(U64 destination,U64 source) const noexcept {
        if(!destination||!source)return true;
        if(destination>=first.load(std::memory_order_acquire)&&destination<last.load(std::memory_order_acquire))return true;
        return Contains(destination)||Contains(source);
    }
};
inline Filter filter;
// AS identities have an independent publication path through the live owner.
// Keep their conservative rejection separate from historical UAV/source tags.
inline Filter acceleration;
inline void MarkAcceleration(U64 address) noexcept {acceleration.Mark(address);filter.Mark(address);}
}
