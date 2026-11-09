#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <atomic>
#include <memory>
#include <vector>
#include <cstdint>
// Allocation lifetime only: references do not freeze buffer contents or prove
// resource states, AS build ordering, descriptor selection, or PSO ownership.
namespace PTOwnedScene {
class LeaseBudget {
    std::atomic<std::uint64_t> held{0};
    const std::uint64_t limit;
public:
    explicit LeaseBudget(std::uint64_t maximum=2ull*1024*1024*1024):limit(maximum){}
    bool Acquire(std::uint64_t bytes) noexcept {
        auto value=held.load();
        do {if(!bytes||value>limit||bytes>limit-value)return false;}
        while(!held.compare_exchange_weak(value,value+bytes));
        return true;
    }
    void Release(std::uint64_t bytes) noexcept {held.fetch_sub(bytes);}
    std::uint64_t Held() const noexcept {return held.load();}
};
}
