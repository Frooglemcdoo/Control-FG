#pragma once
#include <memory>
#include "rr_paired_state.h"
#include <wrl/client.h>

// The early GI pass and the later character composite hold independent GPU leases.
struct RRCharacterGITarget {
    Microsoft::WRL::ComPtr<ID3D12Resource> image[2];
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptors;
    Microsoft::WRL::ComPtr<ID3D12Resource> reference[2],temporal,raw;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> referenceHeap;
    unsigned long long session=0;
    unsigned historyFrames=0;
    UINT width=0,height=0,output=0,passes=0;
    UINT64 frame=0,view=0;
    bool submitted=false,invalid=false;
};
static std::shared_ptr<RRCharacterGITarget> RRCharacterGIForColor(
    UINT64 frame,UINT64 view,ID3D12CommandQueue*,ID3D12GraphicsCommandList*,UINT,UINT) noexcept;
static bool RRCharacterUsesSingleSample() noexcept;
static const char* RRCharacterRecordGISource(UINT64 frame,UINT64 view,ID3D12CommandQueue*,ID3D12GraphicsCommandList*,unsigned passes) noexcept;
inline std::atomic<const char*> rrCharacterGISourceReason{"awaiting_diffuse_gi"};
