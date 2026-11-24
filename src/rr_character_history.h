#pragma once
#include <cmath>
#include <wrl/client.h>

// Both images belong to one RR device/queue epoch. The current guide lease also
// holds each image it references until the existing submission fence completes.
struct RRCharacterHistory {
    Microsoft::WRL::ComPtr<ID3D12Resource> image[2];
    UINT previous=0;
    UINT64 allocationEpoch=~UINT64(0),frame=0;
    void* view=nullptr;
    float jitterX=0,jitterY=0,scaleX=0,scaleY=0;
    bool Prepare(ID3D12Device* device,UINT width,UINT height,UINT64 epoch) noexcept {
        if(allocationEpoch==epoch)return image[0]&&image[1];
        allocationEpoch=epoch;frame=0;previous=0;
        D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;
        heap.CreationNodeMask=heap.VisibleNodeMask=1;
        D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;d.Width=width;d.Height=height;
        d.MipLevels=d.DepthOrArraySize=d.SampleDesc.Count=1;d.Format=DXGI_FORMAT_R32G32B32A32_UINT;
        d.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        for(auto& v:image)v.Reset();
        for(auto& v:image)if(FAILED(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&d,
                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,nullptr,IID_PPV_ARGS(&v)))) {
            for(auto& allocated:image)allocated.Reset();
            return false;
        }
        return true;
    }
};
struct RRCharacterHistoryParams {
    float scaleX=0,scaleY=0,deltaX=0,deltaY=0;
    UINT mode=0;
};
static_assert(sizeof(RRCharacterHistoryParams)==5*sizeof(UINT),"Character history constants");
inline RRCharacterHistoryParams RRCharacterHistoryForFrame(const RRCharacterHistory& h,UINT64 frame,UINT64 evaluated,
        void* view,int reset,float x,float y,float scaleX,float scaleY) noexcept {
    RRCharacterHistoryParams p{scaleX,scaleY,0,0,1};
    if(h.frame && h.frame+1==frame && evaluated==h.frame && !reset && view && h.view==view &&
       scaleX==h.scaleX && scaleY==h.scaleY) {
        p.mode=2;p.deltaX=h.jitterX-x;p.deltaY=h.jitterY-y;
    }
    return p;
}
