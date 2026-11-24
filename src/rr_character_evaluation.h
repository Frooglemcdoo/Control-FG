#pragma once
#include "rr_character_color_binding.h"
static unsigned RRCharacterEvaluate(RREvaluationC call,ID3D12GraphicsCommandList* list,void* feature,
    void* parameters,void* callback,ID3D12Resource* original,ID3D12Resource* replacement,bool& bound) {
    RRNativeParameterApi api{parameters};
    auto evaluate=[&](){return call(list,feature,parameters,callback);};
    const auto result=RRWithCharacterColor(api,original,replacement,evaluate,bound);
    const UINT64 measured=rrCharacterMeasuredFrame.load();
    rrCharacterBlendStatus.store(!rrCharacterBlendRequested.load()?L"Off":
        std::strcmp(rrCharacterGISourceReason.load(),"native_character_gi")==0?
            (measured && rrCharacterRawFrame.load()-measured<=240?
                (rrCharacterMeasuredPixels.load()?L"Character GI verified on GPU":L"Character mask has zero coverage"):
                L"Checking character GI coverage"):L"Waiting for verified character GI");
    return result;
}
