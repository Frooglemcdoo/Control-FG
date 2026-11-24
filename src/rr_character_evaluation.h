#pragma once
#include "rr_character_color_binding.h"
static unsigned RRCharacterEvaluate(RREvaluationC call,ID3D12GraphicsCommandList* list,void* feature,
    void* parameters,void* callback,ID3D12Resource* original,ID3D12Resource* replacement,bool& bound) {
    RRNativeParameterApi api{parameters};
    auto evaluate=[&](){return call(list,feature,parameters,callback);};
    const auto result=RRWithCharacterColor(api,original,replacement,evaluate,bound);
    rrCharacterBlendStatus.store(bound?L"Character color bound to RR":L"Character blend bypassed");
    return result;
}
