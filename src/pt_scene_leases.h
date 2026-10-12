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
class Runtime;
class Submissions;
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
class LeaseBatch {
    friend class Runtime;
    friend class Submissions;
    struct Payload {
        std::shared_ptr<LeaseBudget> budget;
        std::uint64_t bytes=0;
        std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> resources;
        ~Payload(){resources.clear();if(bytes)budget->Release(bytes);}
    };
    std::unique_ptr<Payload> payload;
public:
    LeaseBatch()=default;
    LeaseBatch(LeaseBatch&&) noexcept=default;
    LeaseBatch& operator=(LeaseBatch&&) noexcept=default;
    LeaseBatch(const LeaseBatch&)=delete;
    LeaseBatch& operator=(const LeaseBatch&)=delete;
    std::size_t Count() const noexcept {return payload?payload->resources.size():0;}
    std::uint64_t Bytes() const noexcept {return payload?payload->bytes:0;}
};
}
