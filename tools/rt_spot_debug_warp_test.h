#pragma once
#include <array>
#include <cmath>
#include <cstring>

// Execute the shipped diagnostics on synthetic light IDs and visibility values.
// GPU waits/readback exist only in this standalone validation executable.
static void TestSpotDebugView(ID3D12Device* device, ID3D12RootSignature* root,
    ID3D12PipelineState* pipeline, bool ids, bool bands=false) {
    constexpr UINT extent=8;
    D3D12_HEAP_PROPERTIES gpu{};gpu.Type=D3D12_HEAP_TYPE_DEFAULT;
    D3D12_HEAP_PROPERTIES uploadHeap{};uploadHeap.Type=D3D12_HEAP_TYPE_UPLOAD;
    D3D12_HEAP_PROPERTIES readbackHeap{};readbackHeap.Type=D3D12_HEAP_TYPE_READBACK;
    auto texture=[&](DXGI_FORMAT format,bool output){
        D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        d.Width=extent;d.Height=extent;d.DepthOrArraySize=1;d.MipLevels=1;d.Format=format;d.SampleDesc.Count=1;
        d.Flags=output?D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS:D3D12_RESOURCE_FLAG_NONE;
        ComPtr<ID3D12Resource> r;Check(device->CreateCommittedResource(&gpu,D3D12_HEAP_FLAG_NONE,&d,
            output?D3D12_RESOURCE_STATE_UNORDERED_ACCESS:D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&r)));return r;
    };
    auto buffer=[&](UINT64 bytes,const D3D12_HEAP_PROPERTIES& heap,D3D12_RESOURCE_STATES state){
        D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;d.Width=bytes;
        d.Height=1;d.DepthOrArraySize=1;d.MipLevels=1;d.SampleDesc.Count=1;d.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        ComPtr<ID3D12Resource> r;Check(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&d,state,nullptr,IID_PPV_ARGS(&r)));return r;
    };
    auto light=texture(DXGI_FORMAT_R32G32_UINT,false), visibility=texture(DXGI_FORMAT_R32G32_FLOAT,false);
    auto diffuse=texture(DXGI_FORMAT_R32G32B32A32_FLOAT,true), specular=texture(DXGI_FORMAT_R32G32B32A32_FLOAT,true);
    auto upload=buffer(4096,uploadHeap,D3D12_RESOURCE_STATE_GENERIC_READ);
    auto readback=buffer(4096,readbackHeap,D3D12_RESOURCE_STATE_COPY_DEST);
    unsigned char* mapped=nullptr;D3D12_RANGE empty{0,0};Check(upload->Map(0,&empty,reinterpret_cast<void**>(&mapped)));
    for(UINT y=0;y<extent;++y)for(UINT x=0;x<extent;++x){
        const UINT id=(x==7)?65535:y*extent+x;
        const UINT pair[2]={id,65535};const float v[2]={float((x+y)%5)/4,1-float((x+y)%5)/4};
        memcpy(mapped+y*256+x*8,pair,sizeof(pair));memcpy(mapped+2048+y*256+x*8,v,sizeof(v));
    }
    upload->Unmap(0,nullptr);
    D3D12_COMMAND_QUEUE_DESC qd{};ComPtr<ID3D12CommandQueue> queue;Check(device->CreateCommandQueue(&qd,IID_PPV_ARGS(&queue)));
    ComPtr<ID3D12CommandAllocator> allocator;Check(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)));
    ComPtr<ID3D12GraphicsCommandList> list;Check(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&list)));
    auto transition=[&](ID3D12Resource* resource,D3D12_RESOURCE_STATES before,D3D12_RESOURCE_STATES after){
        D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        b.Transition={resource,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,before,after};list->ResourceBarrier(1,&b);
    };
    auto location=[&](ID3D12Resource* resource,UINT64 offset,DXGI_FORMAT format){
        D3D12_TEXTURE_COPY_LOCATION l{};l.pResource=resource;l.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        l.PlacedFootprint.Offset=offset;l.PlacedFootprint.Footprint={format,extent,extent,1,256};return l;
    };
    ID3D12Resource* inputs[]={light.Get(),visibility.Get()};
    const DXGI_FORMAT formats[]={DXGI_FORMAT_R32G32_UINT,DXGI_FORMAT_R32G32_FLOAT};
    for(UINT i=0;i<2;++i){
        auto src=location(upload.Get(),i*2048,formats[i]);D3D12_TEXTURE_COPY_LOCATION dst{};dst.pResource=inputs[i];
        dst.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;list->CopyTextureRegion(&dst,0,0,0,&src,nullptr);
        transition(inputs[i],D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    }
    D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;hd.NumDescriptors=56;hd.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ComPtr<ID3D12DescriptorHeap> heap;Check(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&heap)));
    const UINT stride=device->GetDescriptorHandleIncrementSize(hd.Type);
    auto handle=[&](UINT index){auto h=heap->GetCPUDescriptorHandleForHeapStart();h.ptr+=SIZE_T(index)*stride;return h;};
    for(UINT i=0;i<2;++i){
        D3D12_SHADER_RESOURCE_VIEW_DESC sd{};sd.Format=formats[i];sd.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;
        sd.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;sd.Texture2D.MipLevels=1;
        device->CreateShaderResourceView(inputs[i],&sd,handle(30+i));
    }
    ID3D12Resource* outputs[]={diffuse.Get(),specular.Get()};
    for(UINT i=0;i<2;++i){D3D12_UNORDERED_ACCESS_VIEW_DESC ud{};ud.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;
        ud.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;device->CreateUnorderedAccessView(outputs[i],nullptr,&ud,handle(48+i));}
    ID3D12DescriptorHeap* heaps[]={heap.Get()};list->SetDescriptorHeaps(1,heaps);list->SetComputeRootSignature(root);
    list->SetComputeRootDescriptorTable(0,heap->GetGPUDescriptorHandleForHeapStart());list->SetPipelineState(pipeline);list->Dispatch(1,1,1);
    for(UINT i=0;i<2;++i){
        transition(outputs[i],D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);
        D3D12_TEXTURE_COPY_LOCATION src{};src.pResource=outputs[i];src.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        auto dst=location(readback.Get(),i*2048,DXGI_FORMAT_R32G32B32A32_FLOAT);list->CopyTextureRegion(&dst,0,0,0,&src,nullptr);
    }
    Check(list->Close());ID3D12CommandList* lists[]={list.Get()};queue->ExecuteCommandLists(1,lists);
    ComPtr<ID3D12Fence> fence;Check(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence)));
    HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr);if(!event)throw std::runtime_error("CreateEvent");
    Check(queue->Signal(fence.Get(),1));Check(fence->SetEventOnCompletion(1,event));
    const auto wait=WaitForSingleObject(event,30000);CloseHandle(event);if(wait!=WAIT_OBJECT_0)throw std::runtime_error("Diagnostic WARP timeout");
    D3D12_RANGE range{0,4096};Check(readback->Map(0,&range,reinterpret_cast<void**>(&mapped)));
    const float factors[]={.61803399f,.41421356f,.73205081f};
    for(UINT y=0;y<extent;++y)for(UINT x=0;x<extent;++x){
        float actual[4],spec[4];memcpy(actual,mapped+y*256+x*16,sizeof(actual));memcpy(spec,mapped+2048+y*256+x*16,sizeof(spec));
        for(UINT c=0;c<4;++c){
            float expected=c==3?1.0f:float((x+y)%5)/4;
            if(c<3 && bands){
                const float v=float((x+y)%5)/4;
                const float color[4][3]={{0,0,1},{1,.25f,0},{1,0,0},{0,1,0}};
                expected=color[v>=.875f?3:v>=.625f?2:v>=.25f?1:0][c];
            }
            if(c<3 && ids){const float value=float(y*extent+x)*factors[c];expected=value-std::floor(value);}
            if(c<3 && x==7)expected=c==1?0.0f:1.0f;
            if(!std::isfinite(actual[c]) || std::abs(actual[c]-expected)>0.0001f || spec[c]!=0)
                throw std::runtime_error("Diagnostic pixel/channel/invalid-ID mismatch");
        }
    }
    readback->Unmap(0,&empty);
    std::cout<<"PASS shipped diagnostic GPU values, coordinates, invalid light marker and cleared specular output\n";
}
