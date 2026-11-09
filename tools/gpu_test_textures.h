#pragma once
#include "gpu_test_device.h"
#include <array>
using Pixel=std::array<float,4>;
using Job=std::array<UINT,4>;
static ComPtr<ID3D12Resource> Texture(Device& gpu,UINT width,UINT height,DXGI_FORMAT format,const void* data,bool output=false) {
    D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;
    D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Width=width;desc.Height=height;desc.DepthOrArraySize=desc.MipLevels=1;desc.Format=format;desc.SampleDesc.Count=1;
    if(output)desc.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    ComPtr<ID3D12Resource> texture;
    Check(gpu.device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,output && !data?D3D12_RESOURCE_STATE_UNORDERED_ACCESS:D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&texture)));
    gpu.retained.push_back(texture);
    if(data) {
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};UINT64 bytes=0;
        gpu.device->GetCopyableFootprints(&desc,0,1,0,&footprint,nullptr,nullptr,&bytes);
        std::vector<unsigned char> padded(static_cast<size_t>(bytes));
        for(UINT y=0;y<height;++y)std::memcpy(padded.data()+size_t(y)*footprint.Footprint.RowPitch,static_cast<const unsigned char*>(data)+size_t(y)*width*16,size_t(width)*16);
        auto upload=gpu.Upload(padded);
        D3D12_TEXTURE_COPY_LOCATION to{};to.pResource=texture.Get();to.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        D3D12_TEXTURE_COPY_LOCATION from{};from.pResource=upload.Get();from.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;from.PlacedFootprint=footprint;
        gpu.list->CopyTextureRegion(&to,0,0,0,&from,nullptr);
        gpu.Transition(texture.Get(),D3D12_RESOURCE_STATE_COPY_DEST,output?D3D12_RESOURCE_STATE_UNORDERED_ACCESS:D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    }
    return texture;
}
static void TextureView(Device& gpu,ID3D12Resource* resource,UINT slot,bool output=false) {
    auto cpu=gpu.views->GetCPUDescriptorHandleForHeapStart();cpu.ptr+=SIZE_T(slot)*gpu.descriptorSize;
    if(output) {
        D3D12_UNORDERED_ACCESS_VIEW_DESC d{};d.Format=resource->GetDesc().Format;d.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;
        gpu.device->CreateUnorderedAccessView(resource,nullptr,&d,cpu);
    } else {
        D3D12_SHADER_RESOURCE_VIEW_DESC d{};d.Format=resource->GetDesc().Format;d.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;
        d.Texture2D.MipLevels=1;d.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        gpu.device->CreateShaderResourceView(resource,&d,cpu);
    }
}
struct Readback { ComPtr<ID3D12Resource> buffer;UINT rowPitch; };
static Readback CopyOutput(Device& gpu,ID3D12Resource* texture) {
    auto desc=texture->GetDesc();D3D12_PLACED_SUBRESOURCE_FOOTPRINT fp{};UINT64 bytes=0;
    gpu.device->GetCopyableFootprints(&desc,0,1,0,&fp,nullptr,nullptr,&bytes);
    auto buffer=gpu.Buffer(bytes,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
    gpu.Transition(texture,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);
    D3D12_TEXTURE_COPY_LOCATION to{};to.pResource=buffer.Get();to.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;to.PlacedFootprint=fp;
    D3D12_TEXTURE_COPY_LOCATION from{};from.pResource=texture;from.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    gpu.list->CopyTextureRegion(&to,0,0,0,&from,nullptr);return {buffer,fp.Footprint.RowPitch};
}
