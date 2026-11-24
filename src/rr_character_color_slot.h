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
            GICharacterMeasurement::Counts n{};
            if(sample.Read(n)){
                const UINT extended=n[4]+n[5], supported=n[2]+n[3]+extended;
                const UINT visible=n[1]+supported;
                Log("GI_CHARACTER_BLEND_R499 gpu_frame=%llu drawn=%u visible=%u supported=%u corrected=%u extended_supported=%u extended_corrected=%u",
                    sample.frame,n[0]+visible,visible,supported,n[3]+n[5],extended,n[5]);
            } else Log("GI_CHARACTER_BLEND_R499 gpu_frame=%llu sample=read_failed",sample.frame);
        }
        sample={};writtenFrame=0;
        for(auto& resource:held)resource.Reset();
    }
};
