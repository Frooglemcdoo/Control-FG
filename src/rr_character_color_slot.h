#pragma once
#include "gi_character_measurement.h"
#include "rr_character_gi_source.h"
#include "rr_character_history.h"
#include "rr_evaluation_inputs.h"
struct RRLiveOwner;
struct RRLiveSlot;
static void RRCharacterBeforeGuideDispatch(RRLiveOwner*,RRLiveSlot*,const RRGuideInputSnapshot*) noexcept;

// Reuse the RR guide slot's existing submission fence.
struct RRCharacterColorSlot {
    Microsoft::WRL::ComPtr<ID3D12Resource> output, held[8];
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> root;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> normal, measured;
    std::shared_ptr<RRCharacterGITarget> sourceLease;
    GICharacterMeasurement sample;
    UINT64 writtenFrame=0;
    void Retire() noexcept {
        if(sample.frame){
            GICharacterMeasurement::Sample s{};
            if(sample.Read(s)){
                const auto& n=s.counts;
                const UINT temporal=s.temporal+s.strongTemporal;
                const UINT supported=n[2]+n[3]+n[4]+temporal+s.sourceCorrected,visible=n[1]+supported;
                Log("GI_CHARACTER_BLEND_R503 gpu_frame=%llu drawn=%u visible=%u supported=%u corrected=%u mixed_color_preserved=%u temporal_corrected=%u strong_temporal=%u source_corrected=%u",
                    sample.frame,n[0]+visible,visible,supported,n[3]+temporal+s.sourceCorrected,n[4],temporal,s.strongTemporal,s.sourceCorrected);
                if(s.location){
                    const UINT xy=s.location-1;
                    Log("GI_CHARACTER_BLEND_R503 mixed_frame=%llu input_xy=%u,%u gi=%.6g,%.6g,%.6g scene_kept=%.6g,%.6g,%.6g rejected_rgb=%.6g,%.6g,%.6g",
                        sample.frame,xy&65535u,xy>>16,s.gi[0],s.gi[1],s.gi[2],s.scene[0],s.scene[1],s.scene[2],
                        s.proposed[0],s.proposed[1],s.proposed[2]);
                }
            } else Log("GI_CHARACTER_BLEND_R503 gpu_frame=%llu sample=read_failed",sample.frame);
        }
        sourceLease.reset();sample={};writtenFrame=0;
        for(auto& resource:held)resource.Reset();
    }
};
