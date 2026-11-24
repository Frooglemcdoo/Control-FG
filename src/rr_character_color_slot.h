#pragma once
#include "gi_character_measurement.h"
#include "rr_evaluation_inputs.h"
struct RRLiveOwner;
struct RRLiveSlot;
static void RRCharacterBeforeGuideDispatch(RRLiveOwner*,RRLiveSlot*,const RRGuideInputSnapshot*) noexcept;

// Reuse the RR guide slot's existing submission fence. No frame history.
struct RRCharacterColorSlot {
    Microsoft::WRL::ComPtr<ID3D12Resource> output, held[4];
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> root;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> normal, measured;
    GICharacterMeasurement sample;
    UINT64 writtenFrame=0;
    void Retire() noexcept {
        if(sample.frame){
            std::array<UINT,4> n{};
            if(sample.Read(n))Log("GI_CHARACTER_BLEND_R498 gpu_frame=%llu drawn=%u visible=%u supported=%u corrected=%u",
                sample.frame,n[0]+n[1]+n[2]+n[3],n[1]+n[2]+n[3],n[2]+n[3],n[3]);
            else Log("GI_CHARACTER_BLEND_R498 gpu_frame=%llu sample=read_failed",sample.frame);
        }
        sample={};writtenFrame=0;
        for(auto& resource:held)resource.Reset();
    }
};
