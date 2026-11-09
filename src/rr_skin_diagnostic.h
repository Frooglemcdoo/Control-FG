#pragma once
// Read-only r20q skin / subsurface investigation with throttled diagnostics.
//
// NVIDIA's DLSS-RR 2.14.1 integration contract defines an optional
// 1-channel FP16 SSS guide at input resolution. The guide is derived from the
// luminance delta between the post-SSS and pre-SSS image (or diffuse-light
// equivalents multiplied by diffuse albedo). A binary character mask is NOT a
// valid replacement for that guide, so this build observes first and does not
// synthesize or bind an SSS resource yet.
//
// r20q samples the natural NGX parameter object on both native-SR and RR
// branches before and after evaluation, validates any surfaced resource shape,
// and correlates it with the audited Character/Hair/Eye replay families. This
// is the evidence needed to choose the next implementation safely:
//   (a) bind Control's native SSS guide if it exists,
//   (b) derive the true guide from native pre/post SSS surfaces, or
//   (c) generate a character mask for a later RR-bypass/composite experiment.
static std::atomic<unsigned long long> rrSkinInputObservations{0};
static std::atomic<unsigned> rrSkinLastMask{0xFFFFFFFFu};

enum : unsigned {
    RRSkinSSSGuide = 1u << 0,
    RRSkinBeforeSSS = 1u << 1,
    RRSkinAfterSSS = 1u << 2,
    RRSkinGBufferSubsurface = 1u << 3,
    RRSkinResponsivity = 1u << 4,
};

struct RRSkinResourceInfo {
    ID3D12Resource* resource=nullptr;
    D3D12_RESOURCE_DESC desc{};
    bool described=false;
};

static bool RRSkinInputSized(const RRSkinResourceInfo& info,unsigned width,unsigned height) noexcept {
    return info.resource && info.described && info.desc.Dimension==D3D12_RESOURCE_DIMENSION_TEXTURE2D &&
           info.desc.Width==width && info.desc.Height==height && info.desc.DepthOrArraySize==1 &&
           info.desc.MipLevels==1 && info.desc.SampleDesc.Count==1;
}

static void RRNativeObserveSkinInputs(void* parameters,unsigned long long frame,const char* stage,
                                      unsigned inputWidth,unsigned inputHeight) noexcept {
    const DWORD saved=GetLastError();
    const char* keys[5]{
        "DLSSD.ScreenSpaceSubsurfaceScatteringGuide",
        "DLSSD.ColorBeforeScreenSpaceSubsurfaceScattering",
        "DLSSD.ColorAfterScreenSpaceSubsurfaceScattering",
        "GBuffer.Subsurface",
        "DLSSD.ResponsivityMask"
    };
    RRSkinResourceInfo info[5]{};
    unsigned status[5]{};
    unsigned mask=0;DWORD fault=0;
    __try {
        if(parameters&&ngxGetResource){
            for(unsigned i=0;i<5;++i){
                ID3D12Resource* resource=nullptr;
                status[i]=ngxGetResource(parameters,keys[i],&resource);
                if(status[i]==1&&resource){
                    info[i].resource=resource;mask|=1u<<i;
                    __try { info[i].desc=resource->GetDesc(); info[i].described=true; }
                    __except(EXCEPTION_EXECUTE_HANDLER) { info[i].described=false; }
                }
            }
        }
    } __except(EXCEPTION_EXECUTE_HANDLER){fault=GetExceptionCode();}

    const UINT character=RRDiffuseFamilyCount(frame,2);
    const UINT hair=RRDiffuseFamilyCount(frame,5);
    const UINT eye=RRDiffuseFamilyCount(frame,6);
    const auto characterInfo=RRDiffuseCharacterInfo(frame);
    const bool guideShape=RRSkinInputSized(info[0],inputWidth,inputHeight) && info[0].desc.Format==DXGI_FORMAT_R16_FLOAT;
    const bool beforeShape=RRSkinInputSized(info[1],inputWidth,inputHeight);
    const bool afterShape=RRSkinInputSized(info[2],inputWidth,inputHeight);
    const bool nativePair=beforeShape&&afterShape&&info[1].desc.Format==info[2].desc.Format;

    const auto sample=rrSkinInputObservations.fetch_add(1,std::memory_order_relaxed)+1;
    const unsigned previousMask=rrSkinLastMask.exchange(mask,std::memory_order_relaxed);
    const bool periodic=(frame%240ull)==0;
    // Do not log on Character/Hair/Eye count changes. Those counters naturally
    // vary almost every frame and previously turned this diagnostic into a
    // synchronous render-thread WriteFile stress test.
    const bool noteworthy=sample<=12||(sample&(sample-1))==0||periodic||previousMask!=mask||fault!=0;
    if(noteworthy){
        Log("RR_SKIN_INPUTS frame=%llu sample=%llu stage=%s input=%ux%u sss_mask=0x%X statuses=%u,%u,%u,%u,%u sss_guide=%p before_sss=%p after_sss=%p gbuffer_subsurface=%p responsivity=%p character=%u hair=%u eye=%u character_batches=%u character_instances=%llu character_fingerprint=0x%016llX guide_shape_valid=%u native_pair_valid=%u read_fault=0x%08lX read_only=1 read_only_observation=1 preset_selectable=E,F,K,L,M sss_injection=0 character_responsivity_experiment=%u",
            frame,sample,stage?stage:"unknown",inputWidth,inputHeight,mask,status[0],status[1],status[2],status[3],status[4],
            info[0].resource,info[1].resource,info[2].resource,info[3].resource,info[4].resource,character,hair,eye,
            characterInfo.batches,characterInfo.instances,characterInfo.fingerprint,unsigned(guideShape),unsigned(nativePair),fault,unsigned(control_rr::RRUserSkinResponsivity()));
        for(unsigned i=0;i<5;++i)if(info[i].resource){
            if(info[i].described){
                Log("RR_SKIN_RESOURCE frame=%llu stage=%s key=%s resource=%p width=%llu height=%u format=%u mips=%u sample_count=%u flags=0x%X input_sized=%u",
                    frame,stage?stage:"unknown",keys[i],info[i].resource,
                    static_cast<unsigned long long>(info[i].desc.Width),info[i].desc.Height,unsigned(info[i].desc.Format),
                    unsigned(info[i].desc.MipLevels),info[i].desc.SampleDesc.Count,unsigned(info[i].desc.Flags),
                    unsigned(RRSkinInputSized(info[i],inputWidth,inputHeight)));
            } else {
                Log("RR_SKIN_RESOURCE frame=%llu stage=%s key=%s resource=%p descriptor_fault_or_unavailable=1",
                    frame,stage?stage:"unknown",keys[i],info[i].resource);
            }
        }

        const char* candidate = guideShape ? "native_sss_guide" :
            nativePair ? "derive_true_sss_guide_from_native_pair" :
            character ? "character_mask_then_bypass_or_responsivity_experiment" : "no_character_or_sss_evidence";
        Log("RR_SKIN_PATH frame=%llu stage=%s candidate=%s direct_guide=%u native_prepost_pair=%u character_present=%u next_mutation=disabled_this_build",
            frame,stage?stage:"unknown",candidate,unsigned(guideShape),unsigned(nativePair),unsigned(character!=0));
    }
    SetLastError(saved);
}
