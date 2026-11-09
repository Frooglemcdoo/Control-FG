#pragma once
#include <d3d12.h>
#include <cstdint>
#include <limits>
namespace native_texture_layout {
struct Mip {
    std::uint64_t compactOffset=0,compactBytes=0,footprintBytes=0,rowBytes=0;
    unsigned rowPitch=0,rows=0,width=0,height=0;
};
struct Layout {
    Mip mips[16]{};unsigned count=0,directMips=0;
    std::uint64_t compactBytes=0,footprintBytes=0;
};
// Metadata only. This does not establish the source codec, content identity,
// resource state or permission to write the destination.
inline bool Build(ID3D12Device* device,const D3D12_RESOURCE_DESC& desc,Layout& out){
    out={};
    if(!device||desc.Dimension!=D3D12_RESOURCE_DIMENSION_TEXTURE2D||desc.DepthOrArraySize!=1||
        desc.SampleDesc.Count!=1||!desc.MipLevels||desc.MipLevels>16)return false;
    D3D12_FEATURE_DATA_FORMAT_INFO format{desc.Format,0};
    if(FAILED(device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_INFO,&format,sizeof(format)))||format.PlaneCount!=1)return false;
    Layout result{};
    for(unsigned i=0;i<desc.MipLevels;++i){
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT fp{};UINT rows=0;UINT64 rowBytes=0,total=0;
        device->GetCopyableFootprints(&desc,i,1,0,&fp,&rows,&rowBytes,&total);
        if(!rows||!rowBytes||!total||total==UINT64_MAX||fp.Footprint.Depth!=1||rowBytes>UINT64_MAX/rows)return false;
        const auto compact=rowBytes*rows;
        if(compact>UINT64_MAX-result.compactBytes||total>UINT64_MAX-result.footprintBytes)return false;
        result.mips[i]={result.compactBytes,compact,total,rowBytes,fp.Footprint.RowPitch,rows,fp.Footprint.Width,fp.Footprint.Height};
        result.compactBytes+=compact;result.footprintBytes+=total;
        if(compact==total)++result.directMips;
    }
    result.count=desc.MipLevels;out=result;return true;
}
}
