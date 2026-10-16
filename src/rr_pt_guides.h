#pragma once
#include <memory>
struct RRPTGuideSource {
    std::shared_ptr<void> owner;
    D3D12_GPU_VIRTUAL_ADDRESS raw=0,surface=0;
    const char* reason="pt_off";
};
// Defined after the PT runtime. No borrowed pointer survives without ownership.
static void RRPTAcquireGuides(unsigned long long frame,UINT width,UINT height,
    ID3D12CommandQueue* queue,RRPTGuideSource& out) noexcept;
