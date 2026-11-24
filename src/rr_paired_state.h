#pragma once
#include <atomic>
#include <cstdint>
#include <mutex>
namespace rr_pair {
inline std::atomic<uint64_t> active{0}, serial{0};
inline std::atomic<const wchar_t*> status{L"Ready"};
inline std::mutex stateMutex;
inline std::atomic<uint64_t> fourRayFrame{0};
inline std::atomic<bool> capturingTail{false};
inline uint64_t Armed(){return active.load();}
inline void Issue(uint64_t id,const wchar_t* label){
    std::lock_guard<std::mutex> lock(stateMutex);
    if(id&&active==id)status=label;
}
inline void Toggle(){
    std::lock_guard<std::mutex> lock(stateMutex);
    if(active){active=0;status=L"Cancelled";return;}
    capturingTail=false;active=++serial;status=L"Waiting for four-ray character frames";
}
}
